#ifndef SG_HARNESS_DETAIL_HUNT_SELECTION_HPP
#define SG_HARNESS_DETAIL_HUNT_SELECTION_HPP

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_system.hpp"

/**
 * @file hunt_selection.hpp
 * @brief The selection rules of specs/029 that both hunt routes decide with:
 * the selection word, the two anchor leaves, the device and leaf split, the
 * raw event code shape, the glob matcher and the catalog walk order
 * (FR-002, FR-005, FR-007).
 *
 * Two callers resolve a selection. `expandHuntSelection` of
 * `source/harness/hunt_cli.cpp` answers the option route and
 * `EventCatalog::expand` of `source/harness/hunt.cpp` the library route.
 * Each keeps the sentence of its own refusal and the splitter that feeds it:
 * the option route names the value in the spelling the option carried and
 * cuts comments out of a `@file`, and the library route names the selection
 * it was handed and refuses the empty line outright. What decides whether a
 * text selects what it claims stands here once, so one text decides the same
 * way on both routes (D-11). Nothing here is public API (D-15); no clock,
 * counter read, `/proc`, sysfs or CPUID enters any declaration here (I-7).
 */

namespace sg::detail
{

// The selection word of D-1 and the two anchor leaves of D-13, I-1: `all`
// names the core-device events, and the two leaves ride every pass as
// denominators and stand out of the candidate list.
inline constexpr std::string_view kAllSelection {"all"};
inline constexpr std::string_view kCyclesLeaf {"cpu-cycles"};
inline constexpr std::string_view kInstrLeaf {"instructions"};

// The slots one hunt pass claims of one device: the event and the two anchor
// leaves of E-04 and I-1. A device that publishes fewer slots admits no event,
// whatever anchors it publishes (FR-011, spec PC-2).
inline constexpr std::size_t kHuntPassSlots {3};

/// @brief E-04, FR-011, spec PC-2 (T164): the one device-limit predicate, the
/// decision both hunt routes make.
///
/// The option route tests it against every device its selection stands on
/// before any plan states a pass, and the plan route against the device of
/// every column. One predicate decides both, so one device answers the same
/// way on either route.
///
/// \post true when a device that publishes `slots` carries an event beside the
///       two anchor leaves of its device
[[nodiscard]] inline auto admitsHuntEvent(const std::size_t slots) noexcept
    -> bool
{
  return slots >= kHuntPassSlots;
}

/// @brief E-04, FR-081 (T164): the cause one device that admits no event
/// states, the bytes `contracts/hunt-cli.md` pins behind its `hunt: ` word.
///
/// Each route prints the line the contract states whole: the option route
/// names the `hunt: ` word beside this cause, and the run route reaches the
/// same word at the print of a refused hunt in `source/harness/cli.cpp`. One
/// device therefore prints one line on either route.
///
/// \post `device '<device>' publishes <slots> slots and a hunt pass needs 3`
[[nodiscard]] inline auto deviceLimitCause(
    const std::string_view device, const std::size_t slots) -> std::string
{
  return "device '" + std::string(device) + "' publishes "
      + std::to_string(slots) + " slots and a hunt pass needs "
      + std::to_string(kHuntPassSlots);
}

/// @brief E-01: the device part of one catalog address, the text the limit
/// list is keyed by.
///
/// `splitAddress` of `runner.cpp:41-49` answers the same split for the same
/// spelling.
///
/// \post the text before the last `/`, or the whole address when it holds none
[[nodiscard]] inline auto devicePartOf(const std::string_view address)
    -> std::string_view
{
  // `substr` cuts at the view's end when the count runs past it, so a text
  // without a separator answers itself from the same call.
  const std::size_t slash = address.find_last_of('/');
  return address.substr(0, slash);
}

/// @brief E-01: the leaf part of one catalog address, the name the anchor
/// rule and the raw-code shape read.
///
/// \post the text after the last `/`, or the whole address when it holds none
[[nodiscard]] inline auto leafPartOf(const std::string_view address)
    -> std::string_view
{
  const std::size_t slash = address.find_last_of('/');
  return slash == std::string_view::npos ? address : address.substr(slash + 1);
}

/// @brief FR-005, D-11: the raw event code shape, the perf spelling of a
/// lower-case `r` and one or more hex digits, lower or upper case.
///
/// The shape is tested before the catalog is consulted, so `r4a03` is refused
/// on a host whose kernel would encode it (spec PC-6). Each route keeps its
/// own reading of the entry the shape tests: the option route cuts the entry
/// to its leaf and drops the trailing glob characters of a pattern, the
/// library route tests the element as its splitter left it.
///
/// \post true when `text` is an `r` followed by one or more hex digits
[[nodiscard]] inline auto hasRawCodeShape(const std::string_view text) noexcept
    -> bool
{
  if (text.size() < 2 || text.front() != 'r') {
    return false;
  }
  for (std::size_t at = 1; at < text.size(); ++at) {
    const char digit = text[at];
    const bool hex = (digit >= '0' && digit <= '9')
        || (digit >= 'a' && digit <= 'f') || (digit >= 'A' && digit <= 'F');
    if (!hex) {
      return false;
    }
  }
  return true;
}

/// @brief FR-007: the case-sensitive match of one text against one pattern:
/// `*` stands for any run of characters including none, `?` for one
/// character, and every other character matches itself.
///
/// The last star carries the backtracking, so one pass over the text decides
/// it.
///
/// \post true when the whole text stands against the whole pattern
[[nodiscard]] inline auto globMatches(const std::string_view pattern,
                                      const std::string_view text) -> bool
{
  std::size_t at = 0;
  std::size_t here = 0;
  std::size_t star = std::string_view::npos;
  std::size_t resume = 0;
  while (at < text.size()) {
    if (here < pattern.size()
        && (pattern[here] == '?' || pattern[here] == text[at]))
    {
      ++at;
      ++here;
    } else if (here < pattern.size() && pattern[here] == '*') {
      star = here;
      ++here;
      resume = at;
    } else if (star != std::string_view::npos) {
      here = star + 1;
      at = ++resume;
    } else {
      return false;
    }
  }
  while (here < pattern.size() && pattern[here] == '*') {
    ++here;
  }
  return here == pattern.size();
}

// FR-014, R-09, N-07: the two halves of the one list-file grammar the hunt
// selection and the `--counter` `@`-form read. `splitElements` cuts a text the
// way spec 029 FR-006 reads one, and `readAddressFileLines` reads one file and
// hands its lines to that same splitter, one level deep. Both halves are
// declared here and defined in `source/harness/hunt_selection.cpp`, so the two
// routes share the separator rule while each keeps the sentence of its own
// refusal beside it.

/// @brief FR-006, FR-014 (033): the elements of one selection text.
///
/// One line ends at a newline, the line cuts at the first `#` the way FR-006
/// reads a `@file`, and the rest of the line answers one element per comma. A
/// line a comment or a blank leaves empty is ignored; a comma that leaves
/// nothing beside it answers an empty element for the caller to refuse.
///
/// \pre none: any text answers its elements, and a text with nothing in it
///      answers none
/// \post every element stands trimmed of the blanks around it, no element
///       carries a `#` or a newline, and an empty element answers a comma that
///       stood beside nothing
[[nodiscard]] auto splitElements(const std::string& text)
    -> std::vector<std::string>;

/// @brief FR-006, FR-014, R-09 (033): the address elements of one `@path`,
///        read one level deep.
///
/// The file's lines are the elements under `splitElements`; a line that names
/// another `@path` is an element like any other, because one level stands and
/// no nested read opens (029 D-14, 033 Out of scope). The reader refuses an
/// unopenable file alone, in one line naming the path as the caller spelled
/// it; an empty element and a file that holds no address are the callers'
/// refusals, each in the sentence of its own option (N-07).
///
/// \pre none: a path the host cannot open answers the refusal line below
/// \post on success the answer holds the file's address elements under
///       `splitElements`, in the order the file carries them, empty file
///       included; on refusal the answer is the one line
///       `'<token>' cannot be opened`
[[nodiscard]] auto readAddressFileLines(const std::string& path,
                                        std::string_view token)
    -> std::expected<std::vector<std::string>, std::string>;

/// @brief The walk order of `printObject` of `source/harness/catalog.cpp`:
/// every descendant of `root`, each object before its own children and
/// siblings in the order the parent reports them.
///
/// The option route reads the walked objects beside the machine object's own
/// entries as the catalog of addresses the host answers (spec PC-1); the
/// library route leaves the root out, whose entries are the clock leaves,
/// which count no event (E-01). Whoever walks filters that; the order of the
/// walk is one rule (spec PC-8).
///
/// \post the answer holds every descendant of `root` exactly once in the
///       depth-first order of the walk, and never `root` itself
[[nodiscard]] inline auto catalogWalkOrder(const counters::Object& root)
    -> std::vector<const counters::Object*>
{
  const auto children = root.children();
  auto pending =
      std::vector<const counters::Object*>(children.rbegin(), children.rend());
  auto order = std::vector<const counters::Object*> {};
  while (!pending.empty()) {
    const counters::Object* walked = pending.back();
    pending.pop_back();
    order.push_back(walked);
    const auto nested = walked->children();
    pending.insert(pending.end(), nested.rbegin(), nested.rend());
  }
  return order;
}

}  // namespace sg::detail

#endif
