#ifndef SG_DBC_HPP
#define SG_DBC_HPP

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
enum class Kind : std::uint8_t
{
  PRECONDITION,
  POSTCONDITION,
  INVARIANT,
  ASSERTION
};

/**
 * @brief Stable identity of a contract violation (FR-021).
 *
 * Captured at the violation site for delivery to observer or default
 * response. All pointers are non-owning and valid for the duration of
 * the response.
 */
struct ViolationRecord
{
  Kind kind {};
  char const* file {};
  unsigned line {};
  char const* message {};
  char const* predicateText {};
};

/**
 * @brief Observer hook type installed via setObserver.
 */
using ViolationObserver = std::function<void(ViolationRecord const&)>;

namespace detail
{

inline auto observerSlot() -> ViolationObserver&
{
  static ViolationObserver slot {};
  return slot;
}

inline auto inResponseFlag() -> bool&
{
  thread_local bool inResponse = false;
  return inResponse;
}

class ResponseGuard
{
  bool& m_flag;

public:
  explicit ResponseGuard(bool& flag)
      : m_flag(flag)
  {
    m_flag = true;
  }

  ~ResponseGuard() { m_flag = false; }

  ResponseGuard(ResponseGuard const&) = delete;
  ResponseGuard(ResponseGuard&&) = delete;
  auto operator=(ResponseGuard const&) -> ResponseGuard& = delete;
  auto operator=(ResponseGuard&&) -> ResponseGuard& = delete;
};

// LCOV_EXCL_START
inline auto kindName(Kind const kind) -> char const*
{
  switch (kind) {
    case Kind::PRECONDITION:
      return "precondition";
    case Kind::POSTCONDITION:
      return "postcondition";
    case Kind::INVARIANT:
      return "invariant";
    case Kind::ASSERTION:
      return "assertion";
    default:
      SG_UNREACHABLE;
      return "unknown";
  }
}

inline auto defaultResponse(ViolationRecord const& record) -> void
{
  static_cast<void>(
      std::fprintf(  // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
          stderr,
          "[%s] %s (predicate: %s) at %s:%u\n",
          kindName(record.kind),
          record.message != nullptr ? record.message : "",
          record.predicateText != nullptr ? record.predicateText : "",
          record.file != nullptr ? record.file : "",
          record.line));
  static_cast<void>(std::fflush(stderr));
}

inline auto reportViolation(ViolationRecord const& record) -> void
{
  // cppcheck-suppress knownConditionTrueFalse
  if (inResponseFlag()) {
    return;
  }
  ResponseGuard const guard {inResponseFlag()};

  auto const& observer = observerSlot();
  if (observer) {
    observer(record);
    return;
  }
  defaultResponse(record);
}

[[noreturn]] inline SG_NOINLINE SG_COLD auto enforce(
    Kind const kind,
    char const* const file,
    unsigned const line,
    char const* const message,
    char const* const predicateText) -> void
{
  reportViolation(ViolationRecord {
      .kind = kind,
      .file = file,
      .line = line,
      .message = message,
      .predicateText = predicateText,
  });
  std::abort();
}

#if SG_CONTRACTS_SEMANTIC == 1
inline SG_NOINLINE SG_COLD auto dispatch(Kind const kind,
                                         char const* const file,
                                         unsigned const line,
                                         char const* const message,
                                         char const* const predicateText)
    -> void
{
  reportViolation(ViolationRecord {
      .kind = kind,
      .file = file,
      .line = line,
      .message = message,
      .predicateText = predicateText,
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
    char const* const predicateText) -> void
{
  reportViolation(ViolationRecord {
      .kind = kind,
      .file = file,
      .line = line,
      .message = message,
      .predicateText = predicateText,
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
inline auto setObserver(ViolationObserver observer) -> void
{
  detail::observerSlot() = std::move(observer);
}

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

// Enforcement entry points are the fuse box (FR-032). Application code sits
// outside them. LCOV_EXCL_START
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
inline auto checkPrecondition(char const (&message)[],
                              char const* const file,
                              unsigned const line,
                              char const* const pred) -> void
{
  detail::dispatch(
      Kind::PRECONDITION, file, line, static_cast<char const*>(message), pred);
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
inline auto checkPostcondition(char const (&message)[],
                               char const* const file,
                               unsigned const line,
                               char const* const pred) -> void
{
  detail::dispatch(
      Kind::POSTCONDITION, file, line, static_cast<char const*>(message), pred);
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
inline auto checkInvariant(char const (&message)[],
                           char const* const file,
                           unsigned const line,
                           char const* const pred) -> void
{
  detail::dispatch(
      Kind::INVARIANT, file, line, static_cast<char const*>(message), pred);
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
inline auto checkAssertion(char const (&message)[],
                           char const* const file,
                           unsigned const line,
                           char const* const pred) -> void
{
  detail::dispatch(
      Kind::ASSERTION, file, line, static_cast<char const*>(message), pred);
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
[[noreturn]] inline auto checkPreconditionAlways(char const (&message)[],
                                                 char const* const file,
                                                 unsigned const line,
                                                 char const* const pred) -> void
{
  detail::enforce(
      Kind::PRECONDITION, file, line, static_cast<char const*>(message), pred);
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
[[noreturn]] inline auto checkPostconditionAlways(char const (&message)[],
                                                  char const* const file,
                                                  unsigned const line,
                                                  char const* const pred)
    -> void
{
  detail::enforce(
      Kind::POSTCONDITION, file, line, static_cast<char const*>(message), pred);
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
[[noreturn]] inline auto checkInvariantAlways(char const (&message)[],
                                              char const* const file,
                                              unsigned const line,
                                              char const* const pred) -> void
{
  detail::enforce(
      Kind::INVARIANT, file, line, static_cast<char const*>(message), pred);
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
[[noreturn]] inline auto checkAssertionAlways(char const (&message)[],
                                              char const* const file,
                                              unsigned const line,
                                              char const* const pred) -> void
{
  detail::enforce(
      Kind::ASSERTION, file, line, static_cast<char const*>(message), pred);
}

// LCOV_EXCL_STOP

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
      ::sg::dbc::checkPreconditionAlways(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#define SG_ENSURE_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::checkPostconditionAlways(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#define SG_INVARIANT_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::checkInvariantAlways(msg, __FILE__, __LINE__, #pred); \
    } \
  } while (false)

#define SG_ASSERT_ALWAYS(pred, msg) \
  do { \
    [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
    static_assert(!_sg_ct_reject, \
                  "compile-time-evaluable constraint must use static_assert, " \
                  "not a runtime SG_* check"); \
    if (!(pred)) [[unlikely]] { \
      ::sg::dbc::checkAssertionAlways(msg, __FILE__, __LINE__, #pred); \
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
        ::sg::dbc::checkPrecondition(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)

#  define SG_ENSURE(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::checkPostcondition(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)

#  define SG_INVARIANT(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::checkInvariant(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)

#  define SG_ASSERT(pred, msg) \
    do { \
      [[maybe_unused]] constexpr bool _sg_ct_reject = SG_CT_REJECT(pred); \
      static_assert(!_sg_ct_reject, \
                    "compile-time-evaluable constraint must use " \
                    "static_assert, " "not a runtime SG_* check"); \
      if (!(pred)) [[unlikely]] { \
        ::sg::dbc::checkAssertion(msg, __FILE__, __LINE__, #pred); \
      } \
    } while (false)
#endif

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

#undef SG_NOINLINE
#undef SG_COLD
#undef SG_UNREACHABLE
#undef SG_TRAP

#endif
