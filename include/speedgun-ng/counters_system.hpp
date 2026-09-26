#ifndef SPEEDGUN_NG_COUNTERS_SYSTEM_HPP
#define SPEEDGUN_NG_COUNTERS_SYSTEM_HPP

#include <expected>
#include <initializer_list>
#include <memory>
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
class SPEEDGUN_NG_EXPORT object
{
public:
  object(const object&) = default;
  auto operator=(const object&) -> object& = default;

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
   * @brief The parent object, null only for the machine root (FR-001).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto parent() const noexcept -> const object*;

  /**
   * @brief This object's catalog entries (FR-001, FR-004).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto counters() const -> std::vector<catalog_entry>;

  /**
   * @brief The child objects in tree order (US3 scenario 1).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto children() const -> std::vector<const object*>;

  /**
   * @brief Resolves one named counter under the requested dimension
   * (FR-005, FR-008).
   *
   * The catalog unit of the named counter must map to `D`; a mismatch
   * is a recoverable error naming both, and a wrong name is a
   * recoverable error carrying near-miss suggestions (at most five,
   * names within edit distance two first, then description word
   * overlaps, catalog order breaking ties). Nothing is guessed
   * (FR-017).
   *
   * \pre none
   * \post none
   */
  template<class D>
  [[nodiscard]] auto counter(std::string_view name) const
      -> std::expected<sg::counters::counter<D>, error>
  {
    auto leaf = detail::resolve_leaf_core(*this, name);
    if (!leaf.has_value()) {
      return std::unexpected(leaf.error());
    }
    const auto mapped = dimension_of(*unit_from_token(leaf->unit));
    if (!mapped.has_value()) {
      return std::unexpected(mapped.error());
    }
    if (mapped->time != D::time_exponent
        || mapped->events != D::events_exponent)
    {
      return std::unexpected(error {
          .message = "counter '" + std::string(name) + "' has unit '"
              + leaf->unit + "' (dimension time^" + std::to_string(mapped->time)
              + " x events^" + std::to_string(mapped->events)
              + "), not the requested dimension",
          .suggestions = {}});
    }
    return sg::counters::counter<D> {std::move(*leaf)};
  }

private:
  friend class system;
  friend auto detail::resolve_leaf_core(const object& obj,
                                        std::string_view name)
      -> std::expected<detail::leaf_core, error>;
  friend auto detail::compile_core(
      const system& sys,
      const target& tg,
      const std::vector<const detail::expr_core*>& exprs)
      -> std::expected<plan, error>;

  explicit object(void* node) noexcept
      : m_node(node)
  {
  }

  void* m_node = nullptr;  // the tree node
};

/**
 * @brief The root handle to the local machine's counting world
 * (E-01).
 *
 * Registration is accepted before the first use; the first use opens
 * the boundary, closes registration, and freezes the catalog, which
 * makes concurrent catalog reads safe by construction (FR-009).
 */
class SPEEDGUN_NG_EXPORT system
{
public:
  system(const system&) = delete;
  auto operator=(const system&) -> system& = delete;

  /**
   * @brief The process-local root handle.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] static auto local() -> system&;

  /**
   * @brief Registers a provider and merges its objects (FR-009).
   *
   * Registration after the system opened is a recoverable error. A
   * duplicate canonical path under one parent, or a duplicate counter
   * name within one object, is a recoverable error leaving the tree
   * unchanged (FR-008). A provider unit token outside the closed
   * mapping is a recoverable error naming the unit (FR-017).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto register_provider(std::unique_ptr<provider_iface> provider)
      -> std::expected<void, error>;

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
      -> std::expected<sg::counters::object, error>;

  /**
   * @brief One equality predicate of an `objects` selection (FR-003).
   */
  struct filter
  {
    std::string_view key;
    std::string_view value;
  };

  /**
   * @brief Selects every object of one kind whose canonical path
   * satisfies all filters (FR-003, C-SYS-2); predicates combine with
   * AND. Defined keys are the ancestor selectors `package` and `core`
   * matched against canonical-path components: key `package`, value
   * `1` matches component `package-1`. An unknown kind or an unknown
   * filter key is a recoverable error (FR-003, FR-008); results print
   * canonical spelling only (FR-002).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto objects(std::string_view kind,
                             std::initializer_list<filter> filters = {})
      -> std::expected<std::vector<const sg::counters::object*>, error>;

private:
  system();
  ~system();

  friend class object;

  [[nodiscard]] auto handle_for(const std::string& canonical)
      -> sg::counters::object&;

  friend auto detail::compile_core(
      const system& sys,
      const target& tg,
      const std::vector<const detail::expr_core*>& exprs)
      -> std::expected<plan, error>;
  friend auto detail::resolve_leaf_core(const sg::counters::object& obj,
                                        std::string_view name)
      -> std::expected<detail::leaf_core, error>;

  struct impl;  // the tree behind the handle
  impl* m_impl = nullptr;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_SYSTEM_HPP
