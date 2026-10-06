// System handle, tree, registration boundary, and resolution
// (specs/007-counters-and-timers, US1/US3).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/core.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{
namespace
{

constexpr std::size_t suggestion_limit = 5;
constexpr int near_miss_distance = 2;

// Levenshtein edit distance, capped: returns `max + 1` once the row
// minimum exceeds `max`, so near-miss probes stay cheap.
[[nodiscard]] auto edit_distance(std::string_view a,
                                 std::string_view b,
                                 const int max) -> int
{
  std::vector<int> previous(b.size() + 1);
  std::vector<int> current(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) {
    previous[j] = static_cast<int>(j);
  }
  for (std::size_t i = 1; i <= a.size(); ++i) {
    current[0] = static_cast<int>(i);
    int row_min = current[0];
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const int substitution = previous[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      current[j] =
          std::min({previous[j] + 1, current[j - 1] + 1, substitution});
      row_min = std::min(row_min, current[j]);
    }
    if (row_min > max) {
      return max + 1;
    }
    previous = current;
  }
  return previous[b.size()];
}

[[nodiscard]] auto split_words(std::string_view text)
    -> std::vector<std::string>
{
  std::vector<std::string> words;
  std::size_t start = 0;
  while (start < text.size()) {
    const auto end = std::min({text.find('_', start),
                               text.find('-', start),
                               text.find(' ', start),
                               text.size()});
    if (end > start) {
      words.emplace_back(text.substr(start, end - start));
    }
    start = end + 1;
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): gcc attributes a
  // never-taken block to the closing brace of a function returning a named
  // local (NRVO), so the epilogue carries no count even though the function
  // runs. `gcov -b` reports `=====` for this line on every build while the
  // `return` on the previous line carries the call count.
  return words;
}  // LCOV_EXCL_LINE

// True when any word of `description` equals a word of `query`.
[[nodiscard]] auto shares_word(std::string_view description,
                               std::string_view query) -> bool
{
  const auto wanted = split_words(query);
  for (const auto& word : split_words(description)) {
    if (std::ranges::find(wanted, word) != wanted.end()) {
      return true;
    }
  }
  return false;
}

// Near-miss suggestions (FR-008): names within edit distance two
// first, then description word overlaps; catalog order breaks ties;
// at most five.
[[nodiscard]] auto near_misses(
    std::string_view query,
    const std::vector<std::pair<std::string, std::string>>& name_description)
    -> std::vector<std::string>
{
  std::vector<std::string> close;
  std::vector<std::string> related;
  for (const auto& [name, description] : name_description) {
    if (edit_distance(query, name, near_miss_distance) <= near_miss_distance) {
      close.push_back(name);
    } else if (shares_word(description, query)) {
      related.push_back(name);
    }
  }
  std::vector<std::string> suggestions = std::move(close);
  suggestions.insert(suggestions.end(), related.begin(), related.end());
  suggestions.resize(std::min(suggestions.size(), suggestion_limit));
  return suggestions;
}

// One seed's leaves, refusing a unit token outside the closed mapping
// and a counter name the object already carries (FR-008, FR-017).
// `root_leaves` holds the machine root's committed leaves plus the batch
// this call has staged, so a machine seed is checked against both.
[[nodiscard]] auto build_leaves(const object_seed& seed,
                                const std::string& path,
                                const bool on_root,
                                const std::vector<leaf_record>& root_leaves,
                                const int provider_index)
    -> std::expected<std::vector<leaf_record>, error>
{
  std::vector<leaf_record> leaves;
  for (const auto& entry : seed.entries) {
    const auto mapped = unit_from_token(entry.unit);
    if (!mapped.has_value()) {
      return std::unexpected(mapped.error());
    }
    const std::string name(entry.name);
    const bool taken = std::ranges::any_of(leaves,
                                           [&](const leaf_record& leaf)
                                           { return leaf.core.name == name; });
    const bool clashes = on_root
        && std::ranges::any_of(root_leaves,
                               [&](const leaf_record& leaf)
                               { return leaf.core.name == name; });
    if (taken || clashes) {
      return std::unexpected(error {.message = "duplicate counter name '" + name
                                        + "' within object '" + path
                                        + "' (FR-008)",
                                    .suggestions = {}});
    }
    // LCOV_EXCL_BR_START : coverage exclusion (T066): the two rethrow edges
    // of the `std::vector` growth inside `push_back`. They exist only if the
    // allocation throws, and the counters tree never grows a leaf vector past
    // the catalog a provider declares.
    leaves.push_back(leaf_record {
        // LCOV_EXCL_BR_LINE
        .core =
            detail::leaf_core {
                .address = path + "/" + name,
                .name = name,
                .description = std::string(entry.description),
                .unit = std::string(entry.unit),
                .avail = entry.avail,
                .mode = entry.mode,
                .frequency_hz = entry.frequency_hz,
                .scaled = entry.scaled,
            },
        .has_ratio_pair = entry.has_ratio_pair,
        .provider_index = provider_index,
    });  // LCOV_EXCL_BR_LINE
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
  return leaves;
}

// True when `alias` already resolves to an object, in the merged table
// or in the batch this call has staged. An alias resolving to two
// objects would hand every lookup of it to whichever registered first,
// so a clash is refused. `map::emplace` would drop the second node and
// leave it unreported (FR-002).
[[nodiscard]] auto alias_is_held(
    const std::map<std::string, std::string>& merged,
    const std::vector<std::pair<std::string, std::string>>& staged,
    const std::string& alias) -> bool
{
  return merged.contains(alias)
      || std::ranges::any_of(staged,
                             [&](const std::pair<std::string, std::string>& one)
                             { return one.first == alias; });
}

// The canonical path split at its separators, so both filter tests
// below read one component list. The tree is frozen once open, so the
// views stay valid for the call (FR-009).
[[nodiscard]] auto path_components(const std::string& path)
    -> std::vector<std::string_view>
{
  std::vector<std::string_view> components;
  std::size_t start = 0;
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the condition can never
  // fail. The loop breaks on the first `find` miss, and every path reaches that
  // break, so the condition is only ever true on entry.
  while (start <= path.size()) {  // LCOV_EXCL_BR_LINE
    const auto end = path.find('/', start);
    const auto stop = end == std::string::npos ? path.size() : end;
    components.push_back(std::string_view(path).substr(start, stop - start));
    if (end == std::string::npos) {  // LCOV_EXCL_BR_LINE
      break;  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
    start = end + 1;
  }  // LCOV_EXCL_BR_LINE
  // LCOV_EXCL_BR_STOP
  // LCOV_EXCL_LINE : coverage exclusion (T066): the NRVO epilogue block of
  // `path_components`, as at the `split_words` epilogue above.
  return components;
}  // LCOV_EXCL_LINE

// True when the canonical path carries the component the filter spells,
// `key-value` (FR-003).
[[nodiscard]] auto path_carries(const std::string& path,
                                const system::filter& one) -> bool
{
  const std::string wanted =
      std::string(one.key) + "-" + std::string(one.value);
  return std::ranges::any_of(path_components(path),
                             [&](const std::string_view component)
                             { return component == wanted; });
}

// True when the filter's key is a defined attribute key for objects of
// `kind` (FR-003). The two ancestor selectors hold for every kind;
// every other key is defined when an object of that kind spells it as
// a canonical-path component prefix, which is how a provider declares
// an attribute key: the path carries the attributes, so a key needs no
// field of its own in the seed. A declared key whose value matches
// nothing stays defined, so a filter that matches nothing selects
// nothing.
[[nodiscard]] auto key_defined(
    const std::map<std::string, std::unique_ptr<tree_node>>& objects,
    const std::string_view kind,
    const system::filter& one) -> bool
{
  if (one.key == "package" || one.key == "core") {
    return true;
  }
  const std::string prefix = std::string(one.key) + "-";
  for (const auto& [path, node] : objects) {
    if (node->kind != kind) {
      continue;
    }
    for (const auto component : path_components(path)) {
      if (component.starts_with(prefix)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace

system::system()
    : m_impl(std::make_unique<impl>())
{
  auto root = std::make_unique<tree_node>();
  root->kind = "machine";
  root->path = "machine";
  root->description = "local machine";
  m_impl->objects.emplace(root->path, std::move(root));
}

system::~system() = default;

auto system::local() -> system&
{
  static system instance;
  return instance;
}

auto system::register_provider(std::unique_ptr<provider_iface> provider)
    -> std::expected<void, error>
{
  if (m_impl->is_open()) {
    return std::unexpected(error {
        .message = "provider registration after the system opened (FR-009)",
        .suggestions = {}});
  }

  // Staging sink: the tree stays unchanged unless the whole provider
  // validates (FR-008).
  struct staging_sink final : object_sink
  {
    std::vector<object_seed> seeds;

    void add_object(const object_seed& seed) override { seeds.push_back(seed); }
  } sink;

  provider->enumerate(sink);

  const int provider_index = static_cast<int>(m_impl->providers.size());
  std::vector<std::unique_ptr<tree_node>> staged;
  // Seeded with the machine root's committed leaves so the whole merge
  // lands in one assignment after every seed validates (FR-008).
  auto root_leaves = m_impl->objects.at("machine")->leaves;
  std::vector<std::pair<std::string, std::string>> staged_aliases;

  for (const auto& seed : sink.seeds) {
    const std::string path(seed.path);
    const bool on_root = path == "machine";
    if (!on_root
        && (m_impl->objects.contains(path)
            || std::ranges::any_of(staged,
                                   [&](const std::unique_ptr<tree_node>& one)
                                   { return one->path == path; })))
    {
      // A path the batch already staged collides the same way a path the
      // merged tree holds does. `map::emplace` keeps the earlier node, so
      // the later one would be dropped while its alias landed on the
      // surviving node (FR-008).
      return std::unexpected(error {.message = "duplicate object path '" + path
                                        + "' under one parent (FR-008)",
                                    .suggestions = {}});
    }

    auto leaves =
        build_leaves(seed, path, on_root, root_leaves, provider_index);
    if (!leaves.has_value()) {
      return std::unexpected(leaves.error());
    }

    if (on_root) {
      root_leaves.insert(root_leaves.end(),
                         std::make_move_iterator(leaves->begin()),
                         std::make_move_iterator(leaves->end()));
      continue;
    }

    auto node = std::make_unique<tree_node>();
    node->kind = std::string(seed.kind);
    node->path = path;
    node->alias = std::string(seed.alias);
    node->description = std::string(seed.description);
    node->provider_index = provider_index;

    if (!seed.alias.empty()) {
      const std::string alias(seed.alias);
      if (alias_is_held(m_impl->aliases, staged_aliases, alias)) {
        return std::unexpected(
            error {.message = "duplicate platform alias '" + alias
                       + "' already held by another " "object (FR-002)",
                   .suggestions = {}});
      }
      staged_aliases.emplace_back(alias, path);
    }
    node->leaves = std::move(*leaves);
    staged.push_back(std::move(node));
  }

  m_impl->objects.at("machine")->leaves = std::move(root_leaves);
  for (auto& node : staged) {
    m_impl->objects.emplace(node->path, std::move(node));
  }
  for (auto& [alias, path] : staged_aliases) {
    m_impl->aliases.emplace(std::move(alias), std::move(path));
  }
  m_impl->providers.push_back(std::move(provider));
  return {};
}

auto system::object(std::string_view path)
    -> std::expected<sg::counters::object, error>
{
  m_impl->ensure_open();
  const std::string canonical = m_impl->canonicalize(path);
  const auto located = m_impl->objects.find(canonical);
  if (located == m_impl->objects.end()) {
    std::vector<std::pair<std::string, std::string>> candidates;
    for (const auto& [object_path, node] : m_impl->objects) {
      candidates.emplace_back(object_path, node->description);
    }
    return std::unexpected(
        error {.message = "no object at path '" + std::string(path) + "'",
               .suggestions = near_misses(path, candidates)});
  }
  return handle_for(canonical);
}

// The time-stamp entry as a shorter spelling of the uniform lookup
// (specs/008-timestamp-counter FR-004). A pure tree walk: it resolves an
// entry and reads nothing, and it deliberately does not call
// `ensure_open()`, so a program may call it and then still register a
// provider (FR-006). The two failure branches are recoverable errors and
// never contract violations, so neither carries a contract macro (FR-007,
// FR-008).
auto system::tsc() const -> std::expected<counter<dim<0, 1>>, error>
{
  const auto* node = m_impl->find("machine");
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the short-circuit arc
  // between the two operands. The `machine` node is seeded by the system
  // itself at construction, so `find` answers it on every host, both
  // with and without a `perf_event_open` the kernel grants; the emptiness
  // operand below is the half every host reaches.
  if (node == nullptr || node->leaves.empty()) {  // LCOV_EXCL_BR_LINE
    return std::unexpected(error {
        .message =
            "no clock provider is registered, so the " "tree " "holds " "no " "ti" "me" "-s" "ta" "mp" " " "entry to " "res" "olv" "e " "(specs/" "008-timestamp-" "counter FR-007)",
        .suggestions = {}});
  }
  // LCOV_EXCL_BR_STOP
  for (const auto& leaf : node->leaves) {
    if (leaf.core.name == "tsc") {
      return counter<dim<0, 1>> {.leaf = leaf.core};
    }
  }
  return std::unexpected(error {
      .message =
          "the catalog publishes no time-stamp " "entry; " "a clock " "provider" " seeds " "it " "wh" "er" "e " "th" "e " "build " "executes" " the " "instructio" "n, and no " "registered" " provider " "did " "(sp" "ecs" "/00" "8-" "time" "st" "amp" "-co" "unt" "er " "FR-" "008" ")",
      .suggestions = {}});
}

auto system::handle_for(const std::string& canonical) -> sg::counters::object&
{
  // One lock covers the lookup and the insert, so a concurrent miss on one
  // address constructs the handle once and hands the same entry to every
  // thread that named it (FR-010).
  const std::lock_guard<std::mutex> guard {m_impl->handles_lock};
  const auto existing = m_impl->handles.find(canonical);
  if (existing != m_impl->handles.end()) {
    return *existing->second;
  }
  // `make_unique` cannot build this handle: `object`'s node constructor
  // is private and `system` is its only friend (T113).
  const auto inserted = m_impl->handles.emplace(
      canonical,
      std::unique_ptr<sg::counters::object>(
          new sg::counters::object(m_impl->objects.at(canonical).get())));
  return *inserted.first->second;
}

auto system::objects(const std::string_view kind,
                     const std::initializer_list<filter> filters)
    -> std::expected<std::vector<const sg::counters::object*>, error>
{
  m_impl->ensure_open();
  bool known_kind = false;
  for (const auto& [path, node] : m_impl->objects) {
    if (node->kind == kind) {
      known_kind = true;
      break;
    }
  }
  if (!known_kind) {
    return std::unexpected(error {.message = "no object of kind '"
                                      + std::string(kind)
                                      + "' in the tree (FR-003)",
                                  .suggestions = {}});
  }
  for (const auto& one : filters) {
    if (!key_defined(m_impl->objects, kind, one)) {
      return std::unexpected(error {.message = "unknown filter key '"
                                        + std::string(one.key) + "' (FR-003)",
                                    .suggestions = {}});
    }
  }
  std::vector<const sg::counters::object*> matches;
  for (const auto& [path, node] : m_impl->objects) {
    if (node->kind != kind) {
      continue;
    }
    bool matches_all = true;
    for (const auto& one : filters) {
      if (!path_carries(path, one)) {
        matches_all = false;
        break;
      }
    }
    if (matches_all) {
      matches.push_back(&handle_for(path));
    }
  }
  return matches;
}

auto object::path() const noexcept -> std::string_view
{
  return static_cast<const tree_node*>(m_node)->path;
}

auto object::alias() const noexcept -> std::string_view
{
  return static_cast<const tree_node*>(m_node)->alias;
}

auto object::kind() const noexcept -> std::string_view
{
  return static_cast<const tree_node*>(m_node)->kind;
}

auto object::description() const noexcept -> std::string_view
{
  return static_cast<const tree_node*>(m_node)->description;
}

auto object::parent() const noexcept -> const object*
{
  const auto* node = static_cast<const tree_node*>(m_node);
  if (node->path == "machine") {
    return nullptr;
  }
  auto& system_ref = system::local();
  const auto slash = node->path.rfind('/');
  const std::string parent_path = slash == std::string::npos
      ? std::string("machine")
      : node->path.substr(0, slash);
  if (system_ref.m_impl->find(parent_path) == nullptr) {
    return nullptr;
  }
  return &system_ref.handle_for(parent_path);
}

auto object::counters() const -> std::vector<catalog_entry>
{
  const auto* node = static_cast<const tree_node*>(m_node);
  std::vector<catalog_entry> entries;
  entries.reserve(node->leaves.size());
  for (const auto& leaf : node->leaves) {
    const auto recognized = unit_from_token(leaf.core.unit);
    SG_REQUIRE(recognized.has_value(),
               "every stored catalog unit maps (FR-017)");
    // The seed carries no target mask, so the mask follows the state: a
    // countable entry counts on both target kinds the mask names, and a
    // refused, absent, scope-refused, or unencodable entry counts on no
    // target, because a gap between the state and the mask is a state the
    // catalog does not publish (FR-021).
    const target_mask targets = leaf.core.avail == availability::countable
        ? target_thread_bit | target_cpu_bit
        : target_mask {0};
    SG_ENSURE((leaf.core.avail != availability::countable) == (targets == 0),
              "a countable entry names at least one target kind and every "
              "other state names none (FR-021)");
    entries.push_back(catalog_entry {
        .name = leaf.core.name,
        .description = leaf.core.description,
        // Unreachable behind the checked precondition; in a build with
        // contract checking compiled out the entry still needs a value.
        .unit = recognized.value_or(unit::none),
        .avail = leaf.core.avail,
        .mode = leaf.core.mode,
        .targets = targets,
        .frequency_hz = leaf.core.frequency_hz,
        .scaled = leaf.core.scaled,
    });
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): the NRVO epilogue block of
  // `object::counters`, as at the `split_words` epilogue above.
  return entries;
}  // LCOV_EXCL_LINE

auto object::children() const -> std::vector<const object*>
{
  const auto* node = static_cast<const tree_node*>(m_node);
  auto& system_ref = system::local();
  std::vector<const object*> direct;
  for (const auto& [object_path, child] : system_ref.m_impl->objects) {
    const bool is_direct = node->path == "machine"
        ? (object_path.find('/') == std::string::npos
           && object_path != "machine")
        : (object_path.size() > node->path.size() + 1
           && object_path.starts_with(node->path + "/")
           && object_path.find('/', node->path.size() + 1)
               == std::string::npos);
    if (is_direct) {
      direct.push_back(&system_ref.handle_for(object_path));
    }
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): the NRVO epilogue block of
  // `object::children`, as at the `split_words` epilogue above.
  return direct;
}  // LCOV_EXCL_LINE

namespace detail
{

auto resolve_leaf_core(const object& obj,
                       std::string_view name) -> std::expected<leaf_core, error>
{
  const auto* node = static_cast<const tree_node*>(obj.m_node);
  for (const auto& leaf : node->leaves) {
    if (leaf.core.name == name) {
      return leaf.core;
    }
  }
  std::vector<std::pair<std::string, std::string>> candidates;
  for (const auto& leaf : node->leaves) {
    candidates.emplace_back(leaf.core.name, leaf.core.description);
  }
  return std::unexpected(error {.message = "object '" + node->path
                                    + "' has no counter named '"
                                    + std::string(name) + "' (FR-008)",
                                .suggestions = near_misses(name, candidates)});
}

}  // namespace detail

}  // namespace sg::counters
