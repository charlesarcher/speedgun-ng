#ifndef SG_VIEWS_ESCAPE_HPP
#define SG_VIEWS_ESCAPE_HPP

#include <string>
#include <string_view>

/**
 * @file escape.hpp
 * @brief The shared escaper the SVG and the HTML writers of the views
 * run their document-derived text through (A-6 of
 * specs/038-report-view-generators/contracts/views-api.md, the
 * escaper of M-C).
 *
 * Nothing here is public API: the header carries no exported symbol,
 * the function answers in the translation units that include it, and
 * the file joins no build target.
 */

namespace sg::detail
{

/**
 * @brief Escape the markup characters of one document-derived string
 * (A-6).
 *
 * The answer spells the HTML/SVG standard named entities. The
 * ampersand stands rewritten first, since the entity spellings the
 * other two rules introduce carry an `&` themselves.
 *
 * \pre none
 * \post the answer holds `text` with every `&` rewritten `&amp;`,
 *       every `<` rewritten `&lt;` and every `>` rewritten `&gt;`; a
 *       string carrying none of the three stands byte-identical
 */
[[nodiscard]] inline auto escapeMarkup(const std::string_view text)
    -> std::string
{
  std::string escaped;
  escaped.reserve(text.size());
  for (const char c : text) {
    switch (c) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      default:
        escaped += c;
        break;
    }
  }
  return escaped;
}

}  // namespace sg::detail

#endif
