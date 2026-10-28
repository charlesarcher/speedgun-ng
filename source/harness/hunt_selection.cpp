// specs/029 FR-006 and specs/033 FR-014, R-09, N-07: the one list-file grammar
// the hunt selection and the `--counter` `@`-form read. The declarations stand
// in `detail/hunt_selection.hpp`, and each route keeps the sentence of its own
// refusal beside the elements this file answers.

#include <expected>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "detail/hunt_selection.hpp"

namespace sg::detail
{

namespace
{

// The whitespace that stands around an address, an element and the `@path` of
// a file, trimmed by hand the way `rejectPercentileList`'s parser reads its
// spaces, the trim landing before the number is read.
[[nodiscard]] auto trimText(const std::string_view text) -> std::string_view
{
  std::size_t first = 0;
  std::size_t last = text.size();
  // The line split above keeps every newline for itself, so a newline stands
  // in no text this cut reads; the classes here are the blanks a line
  // carries inside it.
  const auto blank = [](const char c)
  { return c == ' ' || c == '\t' || c == '\r'; };
  while (first < last && blank(text[first])) {
    ++first;
  }
  while (last > first && blank(text[last - 1])) {
    --last;
  }
  return text.substr(first, last - first);
}

}  // namespace

auto splitElements(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> elements;
  const std::string_view whole {text};
  std::size_t at = 0;
  while (at < whole.size()) {
    const std::size_t found = whole.find('\n', at);
    const std::size_t lineEnd =
        found == std::string_view::npos ? whole.size() : found;
    std::string_view line = whole.substr(at, lineEnd - at);
    at = lineEnd + 1;
    const std::size_t comment = line.find('#');
    if (comment != std::string_view::npos) {
      line = line.substr(0, comment);
    }
    std::size_t piece = 0;
    // The last piece of a line breaks below; a trailing comma leaves no
    // element to push, so reaching the size ends the loop there instead of
    // running an iteration that could only break again.
    while (piece < line.size()) {
      const std::size_t comma = line.find(',', piece);
      const std::size_t pieceEnd =
          comma == std::string_view::npos ? line.size() : comma;
      const std::string_view element =
          trimText(line.substr(piece, pieceEnd - piece));
      if (!element.empty() || comma != std::string_view::npos) {
        elements.push_back(std::string(element));
      }
      if (comma == std::string_view::npos) {
        break;
      }
      piece = pieceEnd + 1;
    }
  }
  return elements;
}

auto readAddressFileLines(const std::string& path, const std::string_view token)
    -> std::expected<std::vector<std::string>, std::string>
{
  std::ifstream file(path);
  if (!file) {
    return std::unexpected("'" + std::string(token) + "' cannot be opened");
  }
  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line;
    text += "\n";
  }
  // One level stands: the elements come from this file alone, and an element
  // that spells another `@path` reaches the caller as the text it is.
  return splitElements(text);
}

}  // namespace sg::detail
