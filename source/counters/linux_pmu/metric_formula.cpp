// ============================================================================
// Metric rows of the vendored event table, parsed once and resolved against
// the open system (specs/021-metric-formulas).
//
// Two passes live here. The parse pass reads each row's formula text into a
// node tree when a directory is first touched: grammar only, no host, so the
// tree is shared by every resolution (D-02). The resolution pass walks that
// tree against one host, expanding metric references, settling the tests a
// conditional asks, reading the literal constants and building the spine of
// catalog leaves the caller composes with (D-03). A row that will not publish
// keeps its refusal kind and its one-line reason, carries no spine, and the
// pass continues with the next row (FR-007).
//
// The public lookup and the catalog listing share this one pass (D-10), and
// a directory's parse and resolution happen at most once per process, which
// is what lets a lookup parse no text and read no counter (FR-019).
// ============================================================================

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "metric_formula.hpp"

#include "../detail/core.hpp"
#include "../detail/pmu.hpp"
#include "metric_tables.hpp"
#include "speedgun-ng/counters_metrics.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters::detail
{
namespace
{

// The deepest metric-reference chain the walk follows. The pinned tree
// nests a handful of levels, so a chain this long means the table loops.
constexpr std::size_t kExpansionLimit = 64;

// The catalog object a refused row is listed under: the root the listing
// walk starts at, which is where a row with no resolved leaf belongs.
constexpr std::string_view kRootObject {"machine"};

[[nodiscard]] auto refusal(const MetricRefusal kind,
                           std::string reason) -> MetricFailure
{
  MetricFailure failed;
  failed.kind = kind;
  failed.reason = std::move(reason);
  return failed;
}

[[nodiscard]] auto malformed(std::string what,
                             const std::size_t offset) -> MetricFailure
{
  return refusal(
      MetricRefusal::MALFORMED,
      "malformed formula: " + what + " at offset " + std::to_string(offset));
}

[[nodiscard]] auto unsupported(std::string what,
                               const std::size_t offset) -> MetricFailure
{
  return refusal(
      MetricRefusal::UNSUPPORTED,
      "unsupported construct " + what + " at offset " + std::to_string(offset));
}

// The reference half of a token, decoded. `device` is empty for a bare
// name and carries the unit half of an `unit@event,modifier@` spelling;
// `modifier` is the text after the first unescaped comma, the text that
// separates two spellings differing only by a modifier (FR-003).
struct Reference
{
  std::string device;
  std::string event;
  std::string modifier;
  std::string privilege;
};

// Strips the backslash escapes the tables carry inside an at-style
// reference, where `\,` and `\=` keep a modifier's commas out of the
// separator and `\-` spells a name the catalog holds with a hyphen.
[[nodiscard]] auto unescape(std::string_view text) -> std::string
{
  std::string decoded;
  decoded.reserve(text.size());
  for (std::size_t at = 0; at < text.size(); ++at) {
    if (text[at] == '\\' && at + 1 < text.size()) {
      ++at;
    }
    decoded.push_back(text[at]);
  }
  return decoded;
}

// Decodes one reference token: the `@` form, its escapes and the `:`
// privilege suffix (FR-003). The privilege reaches no encoded config here:
// the catalog names the event without it, so the suffix is parsed, kept
// beside the reference and dropped when the leaf is looked up (D-01).
[[nodiscard]] auto decodeReference(std::string_view raw) -> Reference
{
  Reference reference;
  const std::size_t colon = raw.find(':');
  if (colon != std::string_view::npos) {
    reference.privilege = std::string(raw.substr(colon + 1));
    raw = raw.substr(0, colon);
  }
  const std::size_t first = raw.find('@');
  if (first == std::string_view::npos) {
    reference.event = unescape(raw);
    return reference;
  }
  reference.device = std::string(raw.substr(0, first));
  const std::size_t last = raw.rfind('@');
  const std::string_view body = raw.substr(first + 1, last - first - 1);
  std::size_t comma = std::string_view::npos;
  for (std::size_t at = 0; at + 1 < body.size(); ++at) {
    if (body[at] == '\\') {
      ++at;
      continue;
    }
    if (body[at] == ',') {
      comma = at;
      break;
    }
  }
  if (comma == std::string_view::npos) {
    reference.event = unescape(body);
  } else {
    reference.event = unescape(body.substr(0, comma));
    reference.modifier = unescape(body.substr(comma + 1));
  }
  return reference;
}

bool isReferenceChar(const char c) noexcept
{
  return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'
      || c == '.' || c == '-';
}

bool isWordChar(const char c) noexcept
{
  return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

// One token of a formula. `text` holds the reference spelling or the
// constant name; `number` holds a numeric literal.
struct Scanned
{
  enum class Kind : std::uint8_t
  {
    END,
    NUMBER,
    REFERENCE,
    CONSTANT,
    LEFT_PAREN,
    RIGHT_PAREN,
    COMMA,
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    GREATER,
    LESS,
    IF,
    ELSE,
  };

  Kind kind = Kind::END;
  std::string text;
  double number = 0.0;
  std::size_t offset = 0;
};

// The single-character operators, or `END` for a character that is not one.
[[nodiscard]] auto operatorKind(const char c) noexcept -> Scanned::Kind
{
  switch (c) {
    case '(':
      return Scanned::Kind::LEFT_PAREN;
    case ')':
      return Scanned::Kind::RIGHT_PAREN;
    case ',':
      return Scanned::Kind::COMMA;
    case '+':
      return Scanned::Kind::PLUS;
    case '-':
      return Scanned::Kind::MINUS;
    case '*':
      return Scanned::Kind::TIMES;
    case '/':
      return Scanned::Kind::DIVIDE;
    case '>':
      return Scanned::Kind::GREATER;
    case '<':
      return Scanned::Kind::LESS;
    default:
      return Scanned::Kind::END;
  }
}

// The end of the reference that starts at `pos`, including an `@…@` body
// with its escapes and a `:` privilege suffix, or npos when an `@` opens a
// modifier that nothing closes (FR-003).
[[nodiscard]] auto scanReferenceEnd(std::string_view text,
                                    std::size_t pos) -> std::size_t
{
  while (pos < text.size()) {
    const char c = text[pos];
    if (isReferenceChar(c)) {
      ++pos;
      continue;
    }
    if (c == '\\' && pos + 1 < text.size()) {
      pos += 2;
      continue;
    }
    if (c == '@') {
      ++pos;
      while (pos < text.size() && text[pos] != '@') {
        pos += text[pos] == '\\' ? 2U : 1U;
      }
      if (pos >= text.size()) {
        return std::string::npos;
      }
      ++pos;
      continue;
    }
    if (c == ':' && pos + 1 < text.size() && isWordChar(text[pos + 1])) {
      ++pos;
      while (pos < text.size() && isWordChar(text[pos])) {
        ++pos;
      }
      continue;
    }
    break;
  }
  return pos;
}

// Reads the numeric literal that starts at `pos` - decimal, hexadecimal and
// exponent form (FR-003) - into `value`, and returns the end of the token,
// or npos when the text is not a number the loader can trust.
[[nodiscard]] auto scanNumber(std::string_view text,
                              std::size_t pos,
                              double& value) -> std::size_t
{
  std::size_t end = pos;
  const bool hexadecimal =
      text.compare(pos, 2, "0x") == 0 || text.compare(pos, 2, "0X") == 0;
  if (hexadecimal) {
    end += 2;
    while (end < text.size()
           && std::isxdigit(static_cast<unsigned char>(text[end])) != 0)
    {
      ++end;
    }
    const std::string literal(text.substr(pos, end - pos));
    value = static_cast<double>(std::strtoull(literal.c_str(), nullptr, 16));
    return end;
  }
  while (end < text.size()) {
    const char c = text[end];
    const bool exponentSign = (c == '+' || c == '-') && end > pos
        && (text[end - 1] == 'e' || text[end - 1] == 'E');
    if (std::isdigit(static_cast<unsigned char>(c)) == 0 && c != '.' && c != 'e'
        && c != 'E' && !exponentSign)
    {
      break;
    }
    ++end;
  }
  const std::string literal(text.substr(pos, end - pos));
  char* stop = nullptr;
  value = std::strtod(literal.c_str(), &stop);
  if (stop != literal.c_str() + literal.size()) {
    return std::string::npos;
  }
  return end;
}

// The token vector's parser. Precedence is the table's own: the conditional
// binds loosest, then addition, then multiplication, then unary minus, so
// `a + b * c if t else d` reads as `(a + (b * c)) if t else d`.
class Parser
{
public:
  explicit Parser(std::vector<Scanned> tokens)
      : m_tokens {std::move(tokens)}
  {
  }

  // The whole token vector; a token left over after the value is malformed.
  [[nodiscard]] auto parse() -> std::expected<FormulaTree, MetricFailure>
  {
    auto root = conditional();
    if (!root.has_value()) {
      return std::unexpected(root.error());
    }
    if (m_tokens[m_at].kind != Scanned::Kind::END) {
      return std::unexpected(malformed("trailing text", m_tokens[m_at].offset));
    }
    FormulaTree tree;
    tree.nodes = std::move(m_nodes);
    tree.root = *root;
    return tree;
  }

private:
  [[nodiscard]] auto token() const -> const Scanned& { return m_tokens[m_at]; }

  [[nodiscard]] auto at(const Scanned::Kind kind) const noexcept -> bool
  {
    return m_tokens[m_at].kind == kind;
  }

  [[nodiscard]] auto add(const FormulaKind kind,
                         const int left = -1,
                         const int right = -1,
                         const int third = -1) -> int
  {
    FormulaNode node;
    node.kind = kind;
    node.left = left;
    node.right = right;
    node.third = third;
    m_nodes.push_back(std::move(node));
    return static_cast<int>(m_nodes.size()) - 1;
  }

  [[nodiscard]] auto node(const int index) -> FormulaNode&
  {
    return m_nodes[static_cast<std::size_t>(index)];
  }

  [[nodiscard]] auto value(const int index) const -> const FormulaNode&
  {
    return m_nodes[static_cast<std::size_t>(index)];
  }

  [[nodiscard]] auto literal(const double value) -> int
  {
    const int index = add(FormulaKind::CONSTANT);
    node(index).value = value;
    return index;
  }

  // `<value> if <test> else <value>` (FR-005).
  [[nodiscard]] auto conditional() -> std::expected<int, MetricFailure>
  {
    auto value = additive();
    if (!value.has_value() || !at(Scanned::Kind::IF)) {
      return value;
    }
    ++m_at;
    auto test = comparison();
    if (!test.has_value()) {
      return std::unexpected(test.error());
    }
    if (!at(Scanned::Kind::ELSE)) {
      return std::unexpected(malformed("expected 'else'", token().offset));
    }
    ++m_at;
    auto other = conditional();
    if (!other.has_value()) {
      return std::unexpected(other.error());
    }
    return add(FormulaKind::CONDITIONAL, *value, *test, *other);
  }

  // The conditional test: one `<` or `>` comparison of two values, or the
  // value itself when it asks a helper (FR-005). A conditional inside the
  // test of a conditional is a refusal, because the tables never spell one
  // and the arms it would need are undefined (D-09).
  [[nodiscard]] auto comparison() -> std::expected<int, MetricFailure>
  {
    auto left = additive();
    if (!left.has_value()) {
      return std::unexpected(left.error());
    }
    const bool greater = at(Scanned::Kind::GREATER);
    if (!greater && !at(Scanned::Kind::LESS)) {
      if (holdsConditional(*left)) {
        return std::unexpected(unsupported(
            "a conditional inside the test of a conditional", token().offset));
      }
      return left;
    }
    ++m_at;
    auto right = additive();
    if (!right.has_value()) {
      return std::unexpected(right.error());
    }
    if (holdsConditional(*left) || holdsConditional(*right)) {
      return std::unexpected(unsupported(
          "a conditional inside the test of a conditional", token().offset));
    }
    return add(
        greater ? FormulaKind::GREATER : FormulaKind::LESS, *left, *right);
  }

  [[nodiscard]] auto holdsConditional(const int index) const -> bool
  {
    const FormulaNode& made = value(index);
    if (made.kind == FormulaKind::CONDITIONAL) {
      return true;
    }
    const auto child = [&](const int part)
    { return part < 0 ? false : holdsConditional(part); };
    return child(made.left) || child(made.right) || child(made.third);
  }

  [[nodiscard]] auto additive() -> std::expected<int, MetricFailure>
  {
    auto left = multiplicative();
    while (left.has_value()
           && (at(Scanned::Kind::PLUS) || at(Scanned::Kind::MINUS)))
    {
      const FormulaKind kind =
          at(Scanned::Kind::PLUS) ? FormulaKind::ADD : FormulaKind::SUBTRACT;
      ++m_at;
      auto right = multiplicative();
      if (!right.has_value()) {
        return std::unexpected(right.error());
      }
      left = add(kind, *left, *right);
    }
    return left;
  }

  [[nodiscard]] auto multiplicative() -> std::expected<int, MetricFailure>
  {
    auto left = unary();
    while (left.has_value()
           && (at(Scanned::Kind::TIMES) || at(Scanned::Kind::DIVIDE)))
    {
      const FormulaKind kind = at(Scanned::Kind::TIMES) ? FormulaKind::MULTIPLY
                                                        : FormulaKind::DIVIDE;
      ++m_at;
      auto right = unary();
      if (!right.has_value()) {
        return std::unexpected(right.error());
      }
      left = add(kind, *left, *right);
    }
    return left;
  }

  [[nodiscard]] auto unary() -> std::expected<int, MetricFailure>
  {
    if (!at(Scanned::Kind::MINUS)) {
      return primary();
    }
    ++m_at;
    auto operand = unary();
    if (!operand.has_value()) {
      return std::unexpected(operand.error());
    }
    return add(FormulaKind::MULTIPLY, literal(-1.0), *operand);
  }

  // A number, a `#` constant, a reference, a function of references, or a
  // parenthesised value.
  [[nodiscard]] auto primary() -> std::expected<int, MetricFailure>
  {
    const Scanned head = token();
    if (head.kind == Scanned::Kind::NUMBER) {
      ++m_at;
      return literal(head.number);
    }
    if (head.kind == Scanned::Kind::CONSTANT) {
      ++m_at;
      const int index = add(FormulaKind::HOST_CONSTANT);
      node(index).spelling = head.text;
      return index;
    }
    if (head.kind == Scanned::Kind::LEFT_PAREN) {
      ++m_at;
      auto inner = conditional();
      if (!inner.has_value()) {
        return std::unexpected(inner.error());
      }
      if (!at(Scanned::Kind::RIGHT_PAREN)) {
        return std::unexpected(malformed("expected ')'", token().offset));
      }
      ++m_at;
      return inner;
    }
    if (head.kind != Scanned::Kind::REFERENCE) {
      return std::unexpected(malformed("expected a value", head.offset));
    }
    ++m_at;
    if (!at(Scanned::Kind::LEFT_PAREN)) {
      return reference(head);
    }
    return function(head);
  }

  [[nodiscard]] auto reference(const Scanned& head) -> int
  {
    const int index = add(FormulaKind::REFERENCE);
    FormulaNode& made = node(index);
    const Reference decoded = decodeReference(head.text);
    made.spelling = head.text;
    made.device = decoded.device;
    made.event = decoded.event;
    made.modifier = decoded.modifier;
    made.privilege = decoded.privilege;
    return index;
  }

  // The five functions of FR-004. `min` and `max` fold any number of
  // arguments pairwise, because the tree writes three arguments to `min`
  // in 23 rows; `d_ratio(x, y)` becomes `x / max(y, 1)`, the integer-count
  // limit the form carries beside it; a helper takes one reference and
  // nothing else, which is why a predicate whose argument is an expression
  // is a nesting refusal by construction: the value it would return has no
  // dimension to carry (D-09).
  [[nodiscard]] auto function(const Scanned& head)
      -> std::expected<int, MetricFailure>
  {
    const bool minimum = head.text == "min";
    const bool maximum = head.text == "max";
    const bool ratio = head.text == "d_ratio";
    const bool event = head.text == "has_event";
    const bool source = head.text == "source_count";
    if (!minimum && !maximum && !ratio && !event && !source) {
      return std::unexpected(
          unsupported("function '" + head.text + "'", head.offset));
    }
    ++m_at;
    std::vector<int> arguments;
    while (!at(Scanned::Kind::RIGHT_PAREN)) {
      auto argument = conditional();
      if (!argument.has_value()) {
        return std::unexpected(argument.error());
      }
      arguments.push_back(*argument);
      if (!at(Scanned::Kind::COMMA)) {
        break;
      }
      ++m_at;
    }
    if (!at(Scanned::Kind::RIGHT_PAREN)) {
      return std::unexpected(malformed("expected ',' or ')'", token().offset));
    }
    ++m_at;
    if (event || source) {
      return helper(head, arguments, event);
    }
    if (arguments.size() < 2) {
      return std::unexpected(
          malformed("'" + head.text + "' needs two arguments", head.offset));
    }
    for (const int argument : arguments) {
      if (nested(value(argument))) {
        return std::unexpected(
            refusal(MetricRefusal::NESTED,
                    "predicate or conditional nested inside '" + head.text
                        + "' at offset " + std::to_string(head.offset)));
      }
    }
    if (ratio) {
      const int guard = add(FormulaKind::MAXIMUM, arguments[1], literal(1.0));
      return add(FormulaKind::DIVIDE, arguments[0], guard);
    }
    int folded = arguments[0];
    for (std::size_t here = 1; here < arguments.size(); ++here) {
      folded = add(minimum ? FormulaKind::MINIMUM : FormulaKind::MAXIMUM,
                   folded,
                   arguments[here]);
    }
    return folded;
  }

  // `has_event` and `source_count` take one reference; anything else is a
  // predicate whose argument needs a value (D-09).
  [[nodiscard]] auto helper(const Scanned& head,
                            const std::vector<int>& arguments,
                            const bool predicate)
      -> std::expected<int, MetricFailure>
  {
    if (arguments.size() != 1) {
      return std::unexpected(
          malformed("'" + head.text + "' takes one argument", head.offset));
    }
    const FormulaNode& argument = value(arguments[0]);
    if (argument.kind != FormulaKind::REFERENCE) {
      return std::unexpected(refusal(
          MetricRefusal::NESTED,
          "predicate argument of '" + head.text + "' is not a reference at "
              + "offset " + std::to_string(head.offset)));
    }
    // The four texts are read before the node is appended: `add` can grow
    // the node vector, which moves the argument node it would copy from.
    const std::string spelling = argument.spelling;
    const std::string device = argument.device;
    const std::string event = argument.event;
    const std::string modifier = argument.modifier;
    const std::string privilege = argument.privilege;
    const int index =
        add(predicate ? FormulaKind::HAS_EVENT : FormulaKind::SOURCE_COUNT);
    FormulaNode& made = node(index);
    made.spelling = spelling;
    made.device = device;
    made.event = event;
    made.modifier = modifier;
    made.privilege = privilege;
    return index;
  }

  // True when the subtree holds a predicate or a conditional, which no
  // function argument may carry (D-09).
  [[nodiscard]] auto nested(const FormulaNode& node) const -> bool
  {
    const bool predicate = node.kind == FormulaKind::HAS_EVENT
        || node.kind == FormulaKind::SOURCE_COUNT
        || node.kind == FormulaKind::CONDITIONAL
        || node.kind == FormulaKind::GREATER || node.kind == FormulaKind::LESS;
    if (predicate) {
      return true;
    }
    const auto child = [&](const int index)
    { return index < 0 ? false : nested(value(index)); };
    return child(node.left) || child(node.right) || child(node.third);
  }

  std::vector<Scanned> m_tokens;
  std::vector<FormulaNode> m_nodes;
  std::size_t m_at = 0;
};

}  // namespace

// Reads one metric row's formula text into its node tree (T020 to T022).
[[nodiscard]] auto parseFormula(std::string_view text)
    -> std::expected<FormulaTree, MetricFailure>
{
  std::vector<Scanned> tokens;
  std::size_t pos = 0;
  while (pos < text.size()) {
    const char c = text[pos];
    if (std::isspace(static_cast<unsigned char>(c)) != 0) {
      ++pos;
      continue;
    }
    const auto op = operatorKind(c);
    if (op != Scanned::Kind::END) {
      Scanned scanned;
      scanned.kind = op;
      scanned.offset = pos;
      tokens.push_back(std::move(scanned));
      ++pos;
      continue;
    }
    if (c == '#') {
      const std::size_t end = scanReferenceEnd(text, pos + 1);
      if (end == pos + 1) {
        return std::unexpected(malformed("empty constant name", pos));
      }
      if (end == std::string::npos) {
        return std::unexpected(malformed("unterminated '@' reference", pos));
      }
      Scanned scanned;
      scanned.kind = Scanned::Kind::CONSTANT;
      scanned.text = std::string(text.substr(pos + 1, end - pos - 1));
      scanned.offset = pos;
      tokens.push_back(std::move(scanned));
      pos = end;
      continue;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_') {
      const std::size_t end = scanReferenceEnd(text, pos);
      if (end == std::string::npos) {
        return std::unexpected(malformed("unterminated '@' reference", pos));
      }
      const std::string_view word = text.substr(pos, end - pos);
      Scanned scanned;
      scanned.offset = pos;
      if (word == "if") {
        scanned.kind = Scanned::Kind::IF;
      } else if (word == "else") {
        scanned.kind = Scanned::Kind::ELSE;
      } else {
        scanned.kind = Scanned::Kind::REFERENCE;
        scanned.text = std::string(word);
      }
      tokens.push_back(std::move(scanned));
      pos = end;
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '.') {
      double value = 0.0;
      const std::size_t end = scanNumber(text, pos, value);
      if (end == std::string::npos) {
        return std::unexpected(malformed("numeric literal", pos));
      }
      Scanned scanned;
      scanned.kind = Scanned::Kind::NUMBER;
      scanned.number = value;
      scanned.offset = pos;
      tokens.push_back(std::move(scanned));
      pos = end;
      continue;
    }
    return std::unexpected(unsupported("'" + std::string(1, c) + "'", pos));
  }
  Scanned end;
  end.kind = Scanned::Kind::END;
  end.offset = pos;
  tokens.push_back(std::move(end));
  return Parser {std::move(tokens)}.parse();
}

// Parses the metric rows of a directory's parsed table (T024, T026): the
// parse refusals are recorded and the row keeps its place, so the listing
// accounts for every row and the load continues (FR-007).
[[nodiscard]] auto parseMetricRows(const std::vector<PmuTableEntry>& table)
    -> ParsedMetricTable
{
  ParsedMetricTable parsed;
  for (const auto& entry : table) {
    if (entry.metricName.empty()) {
      continue;
    }
    ParsedMetricRow row;
    row.name = entry.metricName;
    row.group = entry.group;
    row.scope = entry.unit;
    row.scaleUnit = entry.scaleUnit;
    row.description = entry.description;
    auto tree = parseFormula(entry.metricExpr);
    if (tree.has_value()) {
      row.tree = std::move(*tree);
      row.parsed = true;
    } else {
      row.refusal = tree.error().kind;
      row.reason = std::move(tree.error().reason);
    }
    const auto inserted = parsed.index.emplace(row.name, parsed.rows.size());
    if (!inserted.second) {
      const std::size_t kept = inserted.first->second;
      row.parsed = false;
      row.refusal = MetricRefusal::COLLISION;
      row.reason = "metric name '" + row.name + "' is already carried by row "
          + std::to_string(parsed.rows[kept].name.empty() ? 0 : kept + 1)
          + " of the same directory (FR-011)";
    }
    parsed.rows.push_back(std::move(row));
  }
  return parsed;
}

// The Linux seam: a directory's bytes and the scope rule that places a
// reference are Linux PMU data, so off Linux the metric surface publishes
// nothing and refuses nothing (FR-042).
#ifdef __linux__

[[nodiscard]] auto parsedMetricTable(const std::string& directory)
    -> const ParsedMetricTable&
{
  // Once-per-directory lazy cache, the same shape the event table of the
  // same directory uses (FR-019).
  static std::mutex cacheMutex;
  static std::map<std::string, ParsedMetricTable> cache;

  const std::scoped_lock lock(cacheMutex);
  if (const auto cached = cache.find(directory); cached != cache.end()) {
    return cached->second;
  }
  return cache.emplace(directory, parseMetricRows(pmuLoadTable(directory)))
      .first->second;
}

#endif  // __linux__

namespace
{

// The one-time-per-row mark of the resolution walk: a row that turns up
// again while it sits on the current chain closes a cycle (FR-012).
constexpr std::uint8_t kWhite = 0;
constexpr std::uint8_t kGrey = 1;
constexpr std::uint8_t kBlack = 2;

/// A resolved subtree: the spine the fold reads, the dimension its leaves
/// carry, the availability they disclose, and the catalog object and name
/// the listing and a refusal reason print it under (D-08).
struct Spine
{
  ExprCore core;
  Dimension dimension;
  Availability avail = Availability::COUNTABLE;
  std::string objectPath;
  std::string name;
};

/// One catalog leaf and the object that published it.
struct Leaf
{
  std::string objectPath;
  LeafCore core;
};

/// A spine holding one literal: the scale node carrying no operand, which
/// is how a formula's literal reaches the fold (FR-009).
[[nodiscard]] auto literalSpine(const double value) -> Spine
{
  Spine spine;
  ExprNode node;
  node.kind = 4;
  node.scale = value;
  spine.core.addNode(node);
  return spine;
}

/// The literal a spine holds, when it holds one and nothing else. Only a
/// literal or a constant read from the host reduces a spine to this, so it
/// answers whether the load can settle the value (FR-012).
[[nodiscard]] auto asLiteral(const Spine& spine) -> std::optional<double>
{
  if (spine.core.nodes.size() != 1) {
    return std::nullopt;
  }
  const auto& node = spine.core.nodes[0];
  if (node.kind != 4 || node.left >= 0) {
    return std::nullopt;
  }
  return node.scale;
}

/// A value the load reads as zero. `-Wfloat-equal` bars the direct
/// comparison, and a value that is on neither side of zero is zero.
[[nodiscard]] constexpr auto isZero(const double value) noexcept -> bool
{
  return !(value > 0.0) && !(value < 0.0);
}

[[nodiscard]] constexpr auto isNotZero(const double value) noexcept -> bool
{
  return value > 0.0 || value < 0.0;
}

/// The dimension as the text of a refusal reason spells it.
[[nodiscard]] auto dimensionWords(const Dimension& dimension) -> std::string
{
  return "events^" + std::to_string(dimension.events) + " time^"
      + std::to_string(dimension.time);
}

/// The operand half of a dimension refusal: the reference's own name where
/// the operand is one reference, its dimension where it is a subtree.
[[nodiscard]] auto describe(const FormulaNode& node,
                            const Spine& spine) -> std::string
{
  if (node.kind == FormulaKind::REFERENCE) {
    return "'" + node.spelling + "' (" + dimensionWords(spine.dimension) + ')';
  }
  return "a subtree (" + dimensionWords(spine.dimension) + ')';
}

/// The worse of two availabilities: the first that is not countable, which
/// is what a metric inherits from its leaves (FR-022).
[[nodiscard]] constexpr auto worstAvailability(
    const Availability left, const Availability right) noexcept -> Availability
{
  return left == Availability::COUNTABLE ? right : left;
}

/// The dimension a binary node carries: a product adds the exponents, a
/// quotient subtracts them, and a pair of like terms keeps its own (FR-010).
[[nodiscard]] constexpr auto combineDimension(const FormulaKind kind,
                                              const Dimension& left,
                                              const Dimension& right)
    -> Dimension
{
  if (kind == FormulaKind::MULTIPLY) {
    return Dimension {
        .time = left.time + right.time,
        .events = left.events + right.events,
    };
  }
  if (kind == FormulaKind::DIVIDE) {
    return Dimension {
        .time = left.time - right.time,
        .events = left.events - right.events,
    };
  }
  return left;
}

[[nodiscard]] constexpr auto isArithmetic(const FormulaKind kind) noexcept
    -> bool
{
  return kind == FormulaKind::ADD || kind == FormulaKind::SUBTRACT
      || kind == FormulaKind::MULTIPLY || kind == FormulaKind::DIVIDE
      || kind == FormulaKind::MINIMUM || kind == FormulaKind::MAXIMUM;
}

/// The spine kind code a formula operator lowers to (FR-009).
[[nodiscard]] constexpr auto spineKind(const FormulaKind kind) noexcept
    -> std::uint8_t
{
  switch (kind) {
    case FormulaKind::ADD:
      return 1;
    case FormulaKind::SUBTRACT:
      return 2;
    case FormulaKind::DIVIDE:
      return 3;
    case FormulaKind::MULTIPLY:
      return 4;
    case FormulaKind::MINIMUM:
      return 5;
    default:
      return 6;
  }
}

/// The dimension of one catalog leaf: a duration leaf carries the time
/// exponent, every other leaf counts events, which is what the table's
/// ratios divide (FR-010).
[[nodiscard]] constexpr auto leafDimension(const LeafCore& leaf) noexcept
    -> Dimension
{
  const bool duration = leaf.unit == "nanoseconds" || leaf.unit == "seconds";
  return duration ? Dimension {.time = 1, .events = 0}
                  : Dimension {.time = 0, .events = 1};
}

/// The catalog scope one device token names. The table spells the core
/// class five ways and the catalog spells it `core` (FR-013).
[[nodiscard]] auto deviceLabel(const std::string_view token) -> std::string
{
  if (token.empty() || token == "core" || token == "cpu" || token == "cpu_core"
      || token == "cpu_atom")
  {
    return "core";
  }
  return std::string(token);
}

class Resolver
{
public:
  Resolver(const ParsedMetricTable& table,
           const std::vector<PmuTableEntry>& events,
           const MetricHost& host,
           std::vector<MetricDefinition>& rows)
      : m_table {table}
      , m_events {events}
      , m_host {host}
      , m_rows {rows}
      , m_colour(table.rows.size(), kWhite)
  {
  }

  auto run() -> void
  {
    for (std::size_t index = 0; index < m_table.rows.size(); ++index) {
      if (m_colour[index] == kWhite) {
        resolveRow(index);
      }
    }
  }

  auto resolveRow(const std::size_t index) -> void
  {
    m_colour[index] = kGrey;
    m_chain.push_back(m_table.rows[index].name);
    m_rows[index] = publish(index);
    m_colour[index] = kBlack;
    m_chain.pop_back();
  }

private:
  [[nodiscard]] static auto at(const FormulaTree& tree,
                               const int index) -> const FormulaNode&
  {
    return tree.nodes[static_cast<std::size_t>(index)];
  }

  [[nodiscard]] auto publish(const std::size_t index) -> MetricDefinition
  {
    const ParsedMetricRow& row = m_table.rows[index];
    MetricDefinition def;
    def.name = row.name;
    def.group = row.group;
    def.unit = row.scaleUnit;
    def.description = row.description;
    def.objectPath = std::string(kRootObject);
    if (!row.parsed) {
      def.refusal = row.refusal;
      def.reason = row.reason;
      return def;
    }
    const ScaleRule* rule = nullptr;
    if (!row.scaleUnit.empty()) {
      rule = scaleRuleFor(row.scaleUnit);
      if (rule == nullptr) {
        def.refusal = MetricRefusal::UNKNOWN_SCALE;
        def.reason = "no scale rule carries the ScaleUnit '" + row.scaleUnit
            + "' (FR-018)";
        return def;
      }
    }
    auto spine = walk(row.tree, row.tree.root, row.scope);
    if (!spine.has_value()) {
      const MetricFailure& failed = spine.error();
      def.refusal = failed.kind;
      def.reason = failed.reason;
      def.suggestions = failed.suggestions;
      return def;
    }
    def.published = true;
    def.core = std::move(spine->core);
    def.avail = spine->avail;
    def.time = spine->dimension.time;
    def.events = spine->dimension.events;
    if (!spine->objectPath.empty()) {
      def.objectPath = spine->objectPath;
    }
    if (rule != nullptr) {
      def.core.scaleAll(rule->factor);
    }
    return def;
  }

  auto walk(const FormulaTree& tree,
            const int index,
            const std::string& scope) -> std::expected<Spine, MetricFailure>
  {
    const FormulaNode& node = at(tree, index);
    if (node.kind == FormulaKind::REFERENCE) {
      return referenced(node, scope);
    }
    if (node.kind == FormulaKind::CONSTANT) {
      return literalSpine(node.value);
    }
    if (node.kind == FormulaKind::HOST_CONSTANT) {
      double value = 0.0;
      const auto settled = readFact(node.spelling, value);
      if (!settled.has_value()) {
        return std::unexpected(settled.error());
      }
      return literalSpine(value);
    }
    if (isArithmetic(node.kind)) {
      return arithmetic(tree, index, scope);
    }
    if (node.kind == FormulaKind::HAS_EVENT
        || node.kind == FormulaKind::SOURCE_COUNT)
    {
      const auto settled = fact(tree, index, scope);
      if (!settled.has_value()) {
        return std::unexpected(settled.error());
      }
      return literalSpine(*settled);
    }
    if (node.kind == FormulaKind::CONDITIONAL) {
      const auto decided = decide(tree, index, scope);
      if (!decided.has_value()) {
        return std::unexpected(decided.error());
      }
      return walk(tree, *decided ? node.left : node.third, scope);
    }
    return std::unexpected(
        refusal(MetricRefusal::UNSUPPORTED,
                "a comparison outside a conditional (FR-005)"));
  }

  auto arithmetic(const FormulaTree& tree,
                  const int index,
                  const std::string& scope)
      -> std::expected<Spine, MetricFailure>
  {
    const FormulaNode& node = at(tree, index);
    auto left = walk(tree, node.left, scope);
    if (!left.has_value()) {
      return std::unexpected(left.error());
    }
    auto right = walk(tree, node.right, scope);
    if (!right.has_value()) {
      return std::unexpected(right.error());
    }
    if (node.kind == FormulaKind::DIVIDE) {
      const auto divisor = asLiteral(*right);
      if (divisor.has_value() && isZero(*divisor)) {
        return std::unexpected(refusal(
            MetricRefusal::UNSUPPORTED,
            "the formula divides by zero: "
                + describe(at(tree, node.right), *right) + " (FR-012)"));
      }
    }
    const bool pairing = node.kind == FormulaKind::ADD
        || node.kind == FormulaKind::SUBTRACT
        || node.kind == FormulaKind::MINIMUM
        || node.kind == FormulaKind::MAXIMUM;
    // `max(counter, 1)` and `min(counter, 1)` are the guards the tables
    // spell, where the literal counts in the counter's unit: it takes its
    // sibling's dimension and the row publishes (FR-004, FR-010).
    const bool guarded =
        (node.kind == FormulaKind::MINIMUM || node.kind == FormulaKind::MAXIMUM)
        && asLiteral(*left).has_value() != asLiteral(*right).has_value();
    if (pairing && !guarded
        && (left->dimension.events != right->dimension.events
            || left->dimension.time != right->dimension.time))
    {
      return std::unexpected(refusal(
          MetricRefusal::DIMENSION,
          "terms do not compose: " + describe(at(tree, node.left), *left)
              + " and " + describe(at(tree, node.right), *right)
              + " (FR-010)"));
    }
    Spine out;
    out.core = std::move(left->core);
    const int leftRoot = out.core.root();
    const int rightRoot = splice(out.core, right->core);
    ExprNode spineNode;
    spineNode.kind = spineKind(node.kind);
    spineNode.left = leftRoot;
    spineNode.right = rightRoot;
    out.core.addNode(spineNode);
    out.dimension = guarded
        ? (asLiteral(*left).has_value() ? right->dimension : left->dimension)
        : combineDimension(node.kind, left->dimension, right->dimension);
    out.avail = worstAvailability(left->avail, right->avail);
    out.objectPath =
        left->objectPath.empty() ? right->objectPath : left->objectPath;
    out.name = left->name.empty() ? right->name : left->name;
    return out;
  }

  /// A name the directory carries as a metric expands into that metric
  /// (FR-008); any other name is an event of the catalog (FR-013).
  auto referenced(const FormulaNode& node, const std::string& scope)
      -> std::expected<Spine, MetricFailure>
  {
    const auto metric = m_table.index.find(node.spelling);
    if (metric != m_table.index.end()) {
      return expand(metric->second);
    }
    return leaf(node, scope);
  }

  auto expand(const std::size_t index) -> std::expected<Spine, MetricFailure>
  {
    const std::string& name = m_table.rows[index].name;
    if (m_colour[index] == kGrey) {
      return std::unexpected(
          refusal(MetricRefusal::CYCLE,
                  "metric reference cycle: " + chainText(name) + " (FR-012)"));
    }
    if (m_chain.size() >= kExpansionLimit) {
      return std::unexpected(
          refusal(MetricRefusal::CYCLE,
                  "metric chain longer than " + std::to_string(kExpansionLimit)
                      + " references at '" + name + "' (FR-012)"));
    }
    if (m_colour[index] == kWhite) {
      resolveRow(index);
    }
    const MetricDefinition& done = m_rows[index];
    if (!done.published) {
      MetricFailure failed;
      failed.kind = done.refusal;
      failed.reason = done.reason;
      failed.suggestions = done.suggestions;
      return std::unexpected(std::move(failed));
    }
    Spine out;
    out.core = done.core;
    out.dimension = Dimension {
        .time = done.time,
        .events = done.events,
    };
    out.avail = done.avail;
    out.objectPath = done.objectPath;
    out.name = name;
    return out;
  }

  [[nodiscard]] auto chainText(const std::string& name) const -> std::string
  {
    std::string text;
    for (const auto& step : m_chain) {
      text += step;
      text += " -> ";
    }
    text += name;
    return text;
  }

  auto leaf(const FormulaNode& node,
            const std::string& scope) -> std::expected<Spine, MetricFailure>
  {
    if (!node.modifier.empty()) {
      // A modifier restates the event's own config fields, and a catalog
      // entry encodes the config its row carries (FR-003, FR-031).
      return std::unexpected(refusal(
          MetricRefusal::UNSUPPORTED,
          "the modifier '" + node.modifier + "' of reference '" + node.spelling
              + "' names a config the catalog entry does not carry"));
    }
    Leaf found;
    const auto settled =
        findLeaf(deviceLabel(node.device.empty() ? scope : node.device),
                 node.event,
                 found);
    if (!settled.has_value()) {
      MetricFailure failed;
      failed.kind = MetricRefusal::UNRESOLVED;
      failed.reason =
          "reference '" + node.spelling + "' " + settled.error().message;
      failed.suggestions = settled.error().suggestions;
      return std::unexpected(std::move(failed));
    }
    Spine out;
    spineNodeOfLeaf(out, found);
    out.dimension = leafDimension(found.core);
    out.avail = found.core.avail;
    out.objectPath = found.objectPath;
    out.name = node.event;
    return out;
  }

  auto spineNodeOfLeaf(Spine& spine, const Leaf& found) -> void
  {
    ExprNode node;
    node.kind = 0;
    node.leaf = spine.core.addLeaf(found.core);
    spine.core.addNode(node);
  }

  /// The leaf one reference names, among the catalog objects the scope
  /// reaches, in catalog order (FR-013, FR-014).
  [[nodiscard]] auto findLeaf(const std::string& label,
                              const std::string& event,
                              Leaf& out) const -> std::expected<void, Error>
  {
    const auto objects = m_host.system->objects("pmu");
    if (!objects.has_value()) {
      return std::unexpected(Error {
          .message =
              "names no event: the host publishes no pmu object (FR-014)",
          .suggestions = {},
      });
    }
    std::vector<std::string> suggestions;
    for (const Object* object : *objects) {
      if (!scopeReaches(std::string(object->path()), label)) {
        continue;
      }
      const auto settled = resolveLeafCore(*object, event);
      if (settled.has_value()) {
        out = Leaf {
            .objectPath = std::string(object->path()),
            .core = *settled,
        };
        return {};
      }
      suggestions.insert(suggestions.end(),
                         settled.error().suggestions.begin(),
                         settled.error().suggestions.end());
    }
    return std::unexpected(Error {
        .message = "names no event the scope '" + label + "' reaches (FR-014)",
        .suggestions = suggestions,
    });
  }

  [[nodiscard]] auto readFact(const std::string& name, double& value) const
      -> std::expected<void, MetricFailure>
  {
    const HostFact* fact = hostFactFor(name);
    if (fact == nullptr) {
      return std::unexpected(
          refusal(MetricRefusal::UNKNOWN_CONSTANT,
                  "no reader answers the constant '#" + name + "' (FR-006)"));
    }
    if (!fact->read(*m_host.system, value)) {
      return std::unexpected(
          refusal(MetricRefusal::UNKNOWN_CONSTANT,
                  "this host publishes no value for the constant '#" + name
                      + "' (FR-006)"));
    }
    return {};
  }

  /// The value a conditional test settles to at load: the helpers, the
  /// constants and the arithmetic over them, and nothing that needs a
  /// counter value (D-09).
  [[nodiscard]] auto fact(const FormulaTree& tree,
                          const int index,
                          const std::string& scope)
      -> std::expected<double, MetricFailure>
  {
    const FormulaNode& node = at(tree, index);
    if (node.kind == FormulaKind::CONSTANT) {
      return node.value;
    }
    if (node.kind == FormulaKind::HOST_CONSTANT) {
      double value = 0.0;
      const auto settled = readFact(node.spelling, value);
      if (!settled.has_value()) {
        return std::unexpected(settled.error());
      }
      return value;
    }
    if (node.kind == FormulaKind::HAS_EVENT) {
      return hasEvent(node, scope) ? 1.0 : 0.0;
    }
    if (node.kind == FormulaKind::SOURCE_COUNT) {
      return sourceCount(node);
    }
    if (isArithmetic(node.kind)) {
      auto left = fact(tree, node.left, scope);
      if (!left.has_value()) {
        return std::unexpected(left.error());
      }
      auto right = fact(tree, node.right, scope);
      if (!right.has_value()) {
        return std::unexpected(right.error());
      }
      if (node.kind == FormulaKind::MULTIPLY) {
        return *left * *right;
      }
      if (node.kind == FormulaKind::DIVIDE) {
        return *left / *right;
      }
      if (node.kind == FormulaKind::SUBTRACT) {
        return *left - *right;
      }
      if (node.kind == FormulaKind::MINIMUM) {
        return std::min(*left, *right);
      }
      if (node.kind == FormulaKind::MAXIMUM) {
        return std::max(*left, *right);
      }
      return *left + *right;
    }
    const auto
        nestedTest = refusal(MetricRefusal::UNSUPPORTED,
                             "the conditional test needs the run-time value of "
                             "a " "reference or a nested test (D-09)");
    return std::unexpected(nestedTest);
  }

  [[nodiscard]] auto hasEvent(const FormulaNode& node,
                              const std::string& scope) const -> bool
  {
    Leaf found;
    const auto settled =
        findLeaf(deviceLabel(node.device.empty() ? scope : node.device),
                 node.event,
                 found);
    return settled.has_value() && found.core.avail == Availability::COUNTABLE;
  }

  [[nodiscard]] auto sourceCount(const FormulaNode& node) const -> double
  {
    std::size_t count = 0;
    for (const auto& entry : m_events) {
      if (entry.metricName.empty() && entry.name == node.event) {
        ++count;
      }
    }
    return static_cast<double>(count);
  }

  [[nodiscard]] auto decide(const FormulaTree& tree,
                            const int index,
                            const std::string& scope)
      -> std::expected<bool, MetricFailure>
  {
    const FormulaNode& conditional = at(tree, index);
    const FormulaNode& test = at(tree, conditional.right);
    if (test.kind != FormulaKind::GREATER && test.kind != FormulaKind::LESS) {
      const auto settled = fact(tree, conditional.right, scope);
      if (!settled.has_value()) {
        return std::unexpected(settled.error());
      }
      return isNotZero(*settled);
    }
    auto left = fact(tree, test.left, scope);
    if (!left.has_value()) {
      return std::unexpected(left.error());
    }
    auto right = fact(tree, test.right, scope);
    if (!right.has_value()) {
      return std::unexpected(right.error());
    }
    return test.kind == FormulaKind::GREATER ? *left > *right : *left < *right;
  }

  const ParsedMetricTable& m_table;
  const std::vector<PmuTableEntry>& m_events;
  const MetricHost& m_host;
  std::vector<MetricDefinition>& m_rows;
  std::vector<std::uint8_t> m_colour;
  std::vector<std::string> m_chain;
};

}  // namespace

#ifdef __linux__

/// The error a metric lookup returns for a name no row carries, with the
/// near-miss metric names beside it (FR-016, T038).
[[nodiscard]] auto noSuchMetric(
    const std::string_view name,
    const std::vector<std::pair<std::string, std::string>>& candidates) -> Error
{
  return Error {
      .message = "no metric named '" + std::string(name)
          + "' in the event table of this host (FR-016)",
      .suggestions = nearMisses(name, candidates),
  };
}

auto metricRows(const ParsedMetricTable& table,
                const std::vector<PmuTableEntry>& events,
                const MetricHost& host) -> std::vector<MetricDefinition>
{
  SG_REQUIRE(host.system != nullptr,
             "a metric resolves against an open system (FR-013)");
  std::vector<MetricDefinition> rows(table.rows.size());
  Resolver(table, events, host, rows).run();
  return rows;
}

auto metricDefinition(const ParsedMetricTable& table,
                      const std::vector<PmuTableEntry>& events,
                      const MetricHost& host,
                      const std::string_view name)
    -> std::expected<MetricDefinition, Error>
{
  SG_REQUIRE(host.system != nullptr,
             "a metric resolves against an open system (FR-013)");
  const auto found = table.index.find(std::string(name));
  if (found == table.index.end()) {
    std::vector<std::pair<std::string, std::string>> candidates;
    candidates.reserve(table.rows.size());
    for (const auto& row : table.rows) {
      candidates.emplace_back(row.name, row.description);
    }
    return std::unexpected(noSuchMetric(name, candidates));
  }
  std::vector<MetricDefinition> rows(table.rows.size());
  Resolver(table, events, host, rows).resolveRow(found->second);
  const MetricDefinition& def = rows[found->second];
  if (!def.published) {
    return std::unexpected(Error {
        .message = def.name + ": " + def.reason,
        .suggestions = def.suggestions,
    });
  }
  return def;
}

auto resolvedMetricTable(const MetricHost& host) -> const ResolvedMetricTable&
{
  // Once-per-host-directory cache, the same shape the parse of the same
  // directory uses: a lookup then parses no text and reads no counter
  // (FR-019).
  static std::mutex cacheMutex;
  static std::map<std::string, ResolvedMetricTable> cache;

  const std::scoped_lock lock(cacheMutex);
  if (const auto cached = cache.find(host.directory); cached != cache.end()) {
    return cached->second;
  }
  const ParsedMetricTable& parsed = parsedMetricTable(host.directory);
  ResolvedMetricTable made;
  made.rows = metricRows(parsed, pmuLoadTable(host.directory), host);
  // The parse index already maps a name to the first row that carried it,
  // which is the row a lookup reaches (FR-011).
  made.index = parsed.index;
  return cache.emplace(host.directory, std::move(made)).first->second;
}

auto metricHostCurrent() -> MetricHost
{
  // The selected directory is a function of this CPU, which does not
  // change while the process runs, and the open system is the same
  // singleton the catalog walks (FR-019).
  static const MetricHost kCurrent = []
  {
    MetricHost made;
    made.system = &System::local();
    made.directory = pmuSelectDirectory(pmuIdentCurrent());
    return made;
  }();
  return kCurrent;
}

#endif  // __linux__

}  // namespace sg::counters::detail

namespace sg::counters
{
namespace
{

// The refusal kinds in enumerator order; the text is what the catalog and
// a test print beside a refused row (FR-021).
constexpr std::array<std::string_view, 9> kMetricRefusalNames {
    "MALFORMED",
    "UNSUPPORTED",
    "CYCLE",
    "UNKNOWN_SCALE",
    "UNKNOWN_CONSTANT",
    "NESTED",
    "DIMENSION",
    "UNRESOLVED",
    "COLLISION",
};

/// The name and description pair list the near-miss suggester reads.
[[nodiscard]] auto metricCandidates(
    const std::vector<detail::MetricDefinition>& rows)
    -> std::vector<std::pair<std::string, std::string>>
{
  std::vector<std::pair<std::string, std::string>> candidates;
  candidates.reserve(rows.size());
  for (const auto& row : rows) {
    candidates.emplace_back(row.name, row.description);
  }
  return candidates;
}

}  // namespace

auto metricRefusalName(const MetricRefusal kind) noexcept -> std::string_view
{
  const auto at = static_cast<std::size_t>(kind);
  SG_REQUIRE(at < kMetricRefusalNames.size(),
             "every refusal kind of the table names itself (FR-021)");
  return kMetricRefusalNames[at];
}

auto metricByName(const std::string_view name)
    -> std::expected<MetricView, Error>
{
#ifdef __linux__
  const detail::MetricHost host = detail::metricHostCurrent();
  if (host.directory.empty()) {
    return std::unexpected(Error {
        .message =
            "this CPU matched no event-table directory, so no " "metric " "tabl" "e " "exists to name (FR-017)",
        .suggestions = {},
    });
  }
  const detail::ResolvedMetricTable& table = detail::resolvedMetricTable(host);
  const auto found = table.index.find(std::string(name));
  if (found == table.index.end()) {
    return std::unexpected(
        detail::noSuchMetric(name, metricCandidates(table.rows)));
  }
  const detail::MetricDefinition& def = table.rows[found->second];
  if (!def.published) {
    return std::unexpected(Error {
        .message = def.name + ": " + def.reason,
        .suggestions = def.suggestions,
    });
  }
  MetricView view;
  view.expression.core = def.core;
  view.group = def.group;
  view.unit = def.unit;
  return view;
#else
  return std::unexpected(Error {
      .message =
          "this platform carries no event table, so no metric " "table " "exist" "s " "to name (FR-042)",
      .suggestions = {},
  });
#endif
}

auto catalogMetrics() -> std::vector<MetricListing>
{
  std::vector<MetricListing> listing;
#ifdef __linux__
  const detail::MetricHost host = detail::metricHostCurrent();
  if (host.directory.empty()) {
    return listing;
  }
  const detail::ResolvedMetricTable& table = detail::resolvedMetricTable(host);
  listing.reserve(table.rows.size());
  for (const auto& def : table.rows) {
    MetricListing line;
    line.objectPath = def.objectPath;
    line.name = def.name;
    line.group = def.group;
    line.unit = def.unit;
    line.description = def.description;
    line.avail = def.avail;
    line.refusal = def.refusal;
    line.published = def.published;
    line.reason = def.reason;
    listing.push_back(std::move(line));
  }
#endif
  return listing;
}

}  // namespace sg::counters
