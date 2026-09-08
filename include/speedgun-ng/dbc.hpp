#ifndef SPEEDGUN_NG_DBC_HPP
#define SPEEDGUN_NG_DBC_HPP

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <utility>

#ifndef SG_CONTRACTS_SEMANTIC
#  define SG_CONTRACTS_SEMANTIC 2
#endif

#if defined(__clang__) || defined(__GNUC__)
#  define SG_NOINLINE __attribute__((noinline))
#  define SG_COLD __attribute__((cold))
#  define SG_UNREACHABLE __builtin_unreachable()
#  define SG_TRAP __builtin_trap()
#elif defined(_MSC_VER)
#  define SG_NOINLINE __declspec(noinline)
#  define SG_COLD
#  define SG_UNREACHABLE __assume(0)
#  define SG_TRAP __debugbreak()
#else
#  define SG_NOINLINE
#  define SG_COLD
#  define SG_UNREACHABLE static_cast<void>(0)
#  define SG_TRAP \
    do { \
      volatile int sg_trap_sink = 0; \
      sg_trap_sink = 1; \
      static_cast<void>(sg_trap_sink); \
      std::abort(); \
    } while (false)
#endif

namespace sg::dbc
{

/**
 * @brief Discriminates the four contract kinds.
 */
// NOLINTNEXTLINE(readability-identifier-naming)
enum class Kind : std::uint8_t
{
  precondition,
  postcondition,
  invariant,
  assertion
};

/**
 * @brief Stable identity of a contract violation (FR-021).
 *
 * Captured at the violation site for delivery to observer or default
 * response. All pointers are non-owning and valid for the duration of
 * the response.
 */
// NOLINTNEXTLINE(readability-identifier-naming)
struct ViolationRecord
{
  Kind kind {};
  char const* file {};
  unsigned line {};
  char const* message {};
  char const* predicateText {};  // NOLINT(readability-identifier-naming)
};

/**
 * @brief Observer hook type installed via set_observer.
 */
using violation_observer = std::function<void(ViolationRecord const&)>;

namespace detail
{

inline auto observer_slot() -> violation_observer&
{
  static violation_observer slot {};
  return slot;
}

inline auto in_response_flag() -> bool&
{
  thread_local bool in_response = false;
  return in_response;
}

class response_guard
{
  bool& m_flag;

public:
  explicit response_guard(bool& flag)
      : m_flag(flag)
  {
    m_flag = true;
  }

  ~response_guard() { m_flag = false; }

  response_guard(response_guard const&) = delete;
  response_guard(response_guard&&) = delete;
  auto operator=(response_guard const&) -> response_guard& = delete;
  auto operator=(response_guard&&) -> response_guard& = delete;
};

// LCOV_EXCL_START
inline auto kind_name(Kind const kind) -> char const*
{
  switch (kind) {
    case Kind::precondition:
      return "precondition";
    case Kind::postcondition:
      return "postcondition";
    case Kind::invariant:
      return "invariant";
    case Kind::assertion:
      return "assertion";
    default:
      SG_UNREACHABLE;
      return "unknown";
  }
}

inline auto default_response(ViolationRecord const& record) -> void
{
  static_cast<void>(
      std::fprintf(  // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
          stderr,
          "[%s] %s (predicate: %s) at %s:%u\n",
          kind_name(record.kind),
          record.message != nullptr ? record.message : "",
          record.predicateText != nullptr ? record.predicateText : "",
          record.file != nullptr ? record.file : "",
          record.line));
  static_cast<void>(std::fflush(stderr));
}

inline auto report_violation(ViolationRecord const& record) -> void
{
  // cppcheck-suppress knownConditionTrueFalse
  if (in_response_flag()) {
    return;
  }
  response_guard const guard {in_response_flag()};

  auto const& observer = observer_slot();
  if (observer) {
    observer(record);
    return;
  }
  default_response(record);
}

[[noreturn]] inline SG_NOINLINE SG_COLD auto enforce(
    Kind const kind,
    char const* const file,
    unsigned const line,
    char const* const message,
    char const* const predicate_text) -> void
{
  report_violation(ViolationRecord {
      .kind = kind,
      .file = file,
      .line = line,
      .message = message,
      .predicateText = predicate_text,
  });
  std::abort();
}

#if SG_CONTRACTS_SEMANTIC == 1
inline SG_NOINLINE SG_COLD auto dispatch(Kind const kind,
                                         char const* const file,
                                         unsigned const line,
                                         char const* const message,
                                         char const* const predicate_text)
    -> void
{
  report_violation(ViolationRecord {
      .kind = kind,
      .file = file,
      .line = line,
      .message = message,
      .predicateText = predicate_text,
  });
}
#elif SG_CONTRACTS_SEMANTIC == 3
[[noreturn]] inline SG_NOINLINE SG_COLD auto dispatch(
    Kind const,
    char const* const,
    unsigned const,
    char const* const,
    char const* const) noexcept -> void
{
  SG_TRAP;
  SG_UNREACHABLE;
}
#elif SG_CONTRACTS_SEMANTIC != 0
[[noreturn]] inline SG_NOINLINE SG_COLD auto dispatch(
    Kind const kind,
    char const* const file,
    unsigned const line,
    char const* const message,
    char const* const predicate_text) -> void
{
  report_violation(ViolationRecord {
      .kind = kind,
      .file = file,
      .line = line,
      .message = message,
      .predicateText = predicate_text,
  });
  std::abort();
}
#endif
// LCOV_EXCL_STOP

}  // namespace detail

/**
 * @brief Install the process-wide violation observer (unique hook).
 *
 * The observer (if non-null) is invoked from the violation path in
 * observe and enforce semantics, before any default response. Only one
 * observer may be active at a time.
 *
 * \pre none
 * \post none
 */
inline auto set_observer(violation_observer observer) -> void
{
  detail::observer_slot() = std::move(observer);
}

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

#if SG_CONTRACTS_SEMANTIC != 0
/**
 * @brief Dispatch a precondition violation under the active semantic.
 *
 * Message argument must be a string literal constant (enforced by the
 * char const(&)[] parameter per FR-020). On failure in a terminating
 * semantic the violation response is invoked and execution ends.
 *
 * \pre none
 * \post none
 */
inline auto check_precondition(char const (&message)[],
                               char const* const file,
                               unsigned const line,
                               char const* const pred) -> void
{
  detail::dispatch(
      Kind::precondition, file, line, static_cast<char const*>(message), pred);
}

/**
 * @brief Dispatch a postcondition violation under the active semantic.
 *
 * Message argument must be a string literal constant (enforced by the
 * char const(&)[] parameter per FR-020). On failure in a terminating
 * semantic the violation response is invoked and execution ends.
 *
 * \pre none
 * \post none
 */
inline auto check_postcondition(char const (&message)[],
                                char const* const file,
                                unsigned const line,
                                char const* const pred) -> void
{
  detail::dispatch(
      Kind::postcondition, file, line, static_cast<char const*>(message), pred);
}

/**
 * @brief Dispatch an invariant violation under the active semantic.
 *
 * Message argument must be a string literal constant (enforced by the
 * char const(&)[] parameter per FR-020). On failure in a terminating
 * semantic the violation response is invoked and execution ends.
 *
 * \pre none
 * \post none
 */
inline auto check_invariant(char const (&message)[],
                            char const* const file,
                            unsigned const line,
                            char const* const pred) -> void
{
  detail::dispatch(
      Kind::invariant, file, line, static_cast<char const*>(message), pred);
}

/**
 * @brief Dispatch an in-body assertion violation under the active semantic.
 *
 * Message argument must be a string literal constant (enforced by the
 * char const(&)[] parameter per FR-020). On failure in a terminating
 * semantic the violation response is invoked and execution ends.
 *
 * \pre none
 * \post none
 */
inline auto check_assertion(char const (&message)[],
                            char const* const file,
                            unsigned const line,
                            char const* const pred) -> void
{
  detail::dispatch(
      Kind::assertion, file, line, static_cast<char const*>(message), pred);
}
#endif

/**
 * @brief Dispatch a precondition violation and terminate.
 *
 * Always-on: present in every evaluation semantic, including ignore.
 * Message must be string literal (FR-020). Invokes response then
 * terminates (FR-013 for quick path uses trap instead).
 *
 * \pre none
 * \post none
 */
[[noreturn]] inline auto check_precondition_always(char const (&message)[],
                                                   char const* const file,
                                                   unsigned const line,
                                                   char const* const pred)
    -> void
{
  detail::enforce(
      Kind::precondition, file, line, static_cast<char const*>(message), pred);
}

/**
 * @brief Dispatch a postcondition violation and terminate.
 *
 * Always-on: present in every evaluation semantic, including ignore.
 * Message must be string literal (FR-020). Invokes response then
 * terminates.
 *
 * \pre none
 * \post none
 */
[[noreturn]] inline auto check_postcondition_always(char const (&message)[],
                                                    char const* const file,
                                                    unsigned const line,
                                                    char const* const pred)
    -> void
{
  detail::enforce(
      Kind::postcondition, file, line, static_cast<char const*>(message), pred);
}

/**
 * @brief Dispatch an invariant violation and terminate.
 *
 * Always-on: present in every evaluation semantic, including ignore.
 * Message must be string literal (FR-020). Invokes response then
 * terminates.
 *
 * \pre none
 * \post none
 */
[[noreturn]] inline auto check_invariant_always(char const (&message)[],
                                                char const* const file,
                                                unsigned const line,
                                                char const* const pred) -> void
{
  detail::enforce(
      Kind::invariant, file, line, static_cast<char const*>(message), pred);
}

/**
 * @brief Dispatch an in-body assertion violation and terminate.
 *
 * Always-on: present in every evaluation semantic, including ignore.
 * Message must be string literal (FR-020). Invokes response then
 * terminates.
 *
 * \pre none
 * \post none
 */
[[noreturn]] inline auto check_assertion_always(char const (&message)[],
                                                char const* const file,
                                                unsigned const line,
                                                char const* const pred) -> void
{
  detail::enforce(
      Kind::assertion, file, line, static_cast<char const*>(message), pred);
}

// NOLINTEND(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

}  // namespace sg::dbc

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

/// Compile-time contract layer (FR-022 / FR-023 / FR-024).
///
/// Constraints evaluable at compile time (type properties, template-parameter
/// ranges, size relations) must be expressed with `static_assert`, a concept,
/// or a `constexpr` validator — not a runtime `SG_*` check. Those forms are
/// active in every configuration at zero runtime cost. Constexpr-only
/// interfaces are constrained through this layer and are exempt from runtime
/// enforcement. A runtime `SG_*` check whose predicate is a compile-time-true
/// constant is a compile error.
#if defined(__GNUC__) || defined(__clang__)
#  define SG_CT_REJECT(pred) (__builtin_constant_p(pred) ? (pred) : false)
#else
#  define SG_CT_REJECT(pred) (false)
#endif

#define SG_REQUIRE_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::check_precondition_always(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#define SG_ENSURE_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::check_postcondition_always(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#define SG_INVARIANT_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::check_invariant_always(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#define SG_ASSERT_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::check_assertion_always(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#if SG_CONTRACTS_SEMANTIC == 0
#  define SG_REQUIRE(pred, msg) ((void)0)
#  define SG_ENSURE(pred, msg) ((void)0)
#  define SG_INVARIANT(pred, msg) ((void)0)
#  define SG_ASSERT(pred, msg) ((void)0)
#else
#  define SG_REQUIRE(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::check_precondition(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)

#  define SG_ENSURE(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::check_postcondition(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)

#  define SG_INVARIANT(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::check_invariant(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)

#  define SG_ASSERT(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::check_assertion(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)
#endif

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

#undef SG_NOINLINE
#undef SG_COLD
#undef SG_UNREACHABLE
#undef SG_TRAP

#endif
