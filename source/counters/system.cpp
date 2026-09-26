// System handle, tree, registration boundary, and resolution
// (specs/007-counters-and-timers, US1/US3).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/core.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"

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
  return words;
}

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

}  // namespace

system::system()
    : m_impl(new impl)
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
  if (m_impl->open) {
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
  std::vector<std::pair<std::string, std::string>> staged_aliases;

  for (const auto& seed : sink.seeds) {
    const std::string path(seed.path);
    const bool is_root = path == "machine";
    if (!is_root && m_impl->objects.contains(path)) {
      return std::unexpected(error {.message = "duplicate object path '" + path
                                        + "' under one parent (FR-008)",
                                    .suggestions = {}});
    }

    auto node = is_root ? nullptr : std::make_unique<tree_node>();
    if (node != nullptr) {
      node->kind = std::string(seed.kind);
      node->path = path;
      node->alias = std::string(seed.alias);
      node->description = std::string(seed.description);
      node->provider_index = provider_index;
    }

    std::vector<leaf_record> leaves;
    for (const auto& entry : seed.entries) {
      const auto mapped = unit_from_token(entry.unit);
      if (!mapped.has_value()) {
        return std::unexpected(mapped.error());
      }
      const std::string name(entry.name);
      const bool taken = std::ranges::any_of(
          leaves,
          [&](const leaf_record& leaf) { return leaf.core.name == name; });
      const bool clashes = is_root
          && std::ranges::any_of(m_impl->objects.at("machine")->leaves,
                                 [&](const leaf_record& leaf)
                                 { return leaf.core.name == name; });
      if (taken || clashes) {
        return std::unexpected(error {.message = "duplicate counter name '"
                                          + name + "' within object '" + path
                                          + "' (FR-008)",
                                      .suggestions = {}});
      }
      leaves.push_back(leaf_record {
          .core =
              detail::leaf_core {
                  .address = path + "/" + name,
                  .name = name,
                  .description = std::string(entry.description),
                  .unit = std::string(entry.unit),
                  .avail = entry.avail,
                  .mode = entry.mode,
              },
          .has_ratio_pair = false,
          .provider_index = provider_index,
      });
    }

    if (is_root) {
      auto& root = m_impl->objects.at("machine");
      root->leaves.insert(root->leaves.end(),
                          std::make_move_iterator(leaves.begin()),
                          std::make_move_iterator(leaves.end()));
      continue;
    }
    if (!seed.alias.empty()) {
      staged_aliases.emplace_back(std::string(seed.alias), path);
    }
    node->leaves = std::move(leaves);
    staged.push_back(std::move(node));
  }

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

auto system::handle_for(const std::string& canonical) -> sg::counters::object&
{
  const auto existing = m_impl->handles.find(canonical);
  if (existing != m_impl->handles.end()) {
    return *existing->second;
  }
  const auto inserted = m_impl->handles.emplace(
      canonical,
      std::unique_ptr<sg::counters::object>(
          new sg::counters::object(m_impl->objects.at(canonical).get())));
  return *inserted.first->second;
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
    const auto mapped = unit_from_token(leaf.core.unit);
    entries.push_back(catalog_entry {
        .name = leaf.core.name,
        .description = leaf.core.description,
        .unit = mapped.value_or(unit::none),
        .avail = leaf.core.avail,
        .mode = leaf.core.mode,
    });
  }
  return entries;
}

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
  return direct;
}

namespace detail
{

auto resolve_leaf_core(const object& obj, std::string_view name)
    -> std::expected<leaf_core, error>
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
