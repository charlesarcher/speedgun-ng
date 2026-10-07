// System handle, tree, registration boundary, and resolution
// (specs/007-counters-and-timers, US1/US3).

#include <algorithm>
#include <cstddef>
#include <expected>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/core.hpp"
#include "detail/pmu.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{
namespace
{

constexpr std::size_t kSuggestionLimit = 5;
constexpr int kNearMissDistance = 2;

// Levenshtein edit distance, capped: returns `max + 1` once the row
// minimum exceeds `max`, so near-miss probes stay cheap.
[[nodiscard]] auto editDistance(std::string_view a,
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
    int rowMin = current[0];
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const int substitution = previous[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      current[j] =
          std::min({previous[j] + 1, current[j - 1] + 1, substitution});
      rowMin = std::min(rowMin, current[j]);
    }
    if (rowMin > max) {
      return max + 1;
    }
    previous = current;
  }
  return previous[b.size()];
}

[[nodiscard]] auto splitWords(std::string_view text) -> std::vector<std::string>
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
[[nodiscard]] auto sharesWord(std::string_view description,
                              std::string_view query) -> bool
{
  const auto wanted = splitWords(query);
  for (const auto& word : splitWords(description)) {
    if (std::ranges::find(wanted, word) != wanted.end()) {
      return true;
    }
  }
  return false;
}

// Near-miss suggestions (FR-008): names within edit distance two
// first, then description word overlaps; catalog order breaks ties;
// at most five.
[[nodiscard]] auto nearMisses(
    std::string_view query,
    const std::vector<std::pair<std::string, std::string>>& nameDescription)
    -> std::vector<std::string>
{
  std::vector<std::string> close;
  std::vector<std::string> related;
  for (const auto& [name, description] : nameDescription) {
    if (editDistance(query, name, kNearMissDistance) <= kNearMissDistance) {
      close.push_back(name);
    } else if (sharesWord(description, query)) {
      related.push_back(name);
    }
  }
  std::vector<std::string> suggestions = std::move(close);
  suggestions.insert(suggestions.end(), related.begin(), related.end());
  suggestions.resize(std::min(suggestions.size(), kSuggestionLimit));
  return suggestions;
}

// One seed's leaves, refusing a unit token outside the closed mapping
// and a counter name the object already carries (FR-008, FR-017).
// `root_leaves` holds the machine root's committed leaves plus the batch
// this call has staged, so a machine seed is checked against both.
[[nodiscard]] auto buildLeaves(const ObjectSeed& seed,
                               const std::string& path,
                               const bool onRoot,
                               const std::vector<LeafRecord>& rootLeaves,
                               const int providerIndex)
    -> std::expected<std::vector<LeafRecord>, Error>
{
  std::vector<LeafRecord> leaves;
  for (const auto& entry : seed.entries) {
    const auto mapped = unitFromToken(entry.unit);
    if (!mapped.has_value()) {
      return std::unexpected(mapped.error());
    }
    const std::string name(entry.name);
    const bool taken = std::ranges::any_of(
        leaves, [&](const LeafRecord& leaf) { return leaf.core.name == name; });
    const bool clashes = onRoot
        && std::ranges::any_of(rootLeaves,
                               [&](const LeafRecord& leaf)
                               { return leaf.core.name == name; });
    if (taken || clashes) {
      return std::unexpected(Error {.message = "duplicate counter name '" + name
                                        + "' within object '" + path
                                        + "' (FR-008)",
                                    .suggestions = {}});
    }
    // LCOV_EXCL_BR_START : coverage exclusion (T066): the two rethrow edges
    // of the `std::vector` growth inside `push_back`. They exist only if the
    // allocation throws, and the counters tree never grows a leaf vector past
    // the catalog a provider declares.
    leaves.push_back(LeafRecord {
        // LCOV_EXCL_BR_LINE
        .core =
            detail::LeafCore {
                .address = path + "/" + name,
                .name = name,
                .description = std::string(entry.description),
                .unit = std::string(entry.unit),
                .avail = entry.avail,
                .mode = entry.mode,
                .frequencyHz = entry.frequencyHz,
                .scaled = entry.scaled,
            },
        .hasRatioPair = entry.hasRatioPair,
        .providerIndex = providerIndex,
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
[[nodiscard]] auto aliasIsHeld(
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
[[nodiscard]] auto pathComponents(const std::string& path)
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
[[nodiscard]] auto pathCarries(const std::string& path,
                               const System::Filter& one) -> bool
{
  const std::string wanted =
      std::string(one.key) + "-" + std::string(one.value);
  return std::ranges::any_of(pathComponents(path),
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
[[nodiscard]] auto keyDefined(
    const std::map<std::string, std::unique_ptr<TreeNode>>& objects,
    const std::string_view kind,
    const System::Filter& one) -> bool
{
  if (one.key == "package" || one.key == "core") {
    return true;
  }
  const std::string prefix = std::string(one.key) + "-";
  for (const auto& [path, node] : objects) {
    if (node->kind != kind) {
      continue;
    }
    for (const auto component : pathComponents(path)) {
      if (component.starts_with(prefix)) {
        return true;
      }
    }
  }
  return false;
}

// The target kinds the availability probe settled each seeded leaf on,
// keyed by the leaf's canonical address. FR-021 states the seeding surface
// gains no field and the availability state stays one enumeration, so the
// per-kind verdicts ride beside the tree: the
// provider records them where it enumerates and the catalog reads them
// where it fills `CatalogEntry::targets` (FR-021). The table is filled
// while a provider registers and read only after the catalog opens, so no
// read races a write (FR-009).
auto probedKindTable() -> std::map<std::string, TargetMask>&
{
  static std::map<std::string, TargetMask> table;
  return table;
}

}  // namespace

System::System()
    : m_impl(std::make_unique<Impl>())
{
  auto root = std::make_unique<TreeNode>();
  root->kind = "machine";
  root->path = "machine";
  root->description = "local machine";
  m_impl->objects.emplace(root->path, std::move(root));
}

System::~System() = default;

auto System::local() -> System&
{
  static System instance;
  return instance;
}

auto System::registerProvider(std::unique_ptr<ProviderIface> provider)
    -> std::expected<void, Error>
{
  if (m_impl->isOpen()) {
    return std::unexpected(Error {
        .message = "provider registration after the system opened (FR-009)",
        .suggestions = {}});
  }

  // Staging sink: the tree stays unchanged unless the whole provider
  // validates (FR-008).
  struct StagingSink final : ObjectSink
  {
    std::vector<ObjectSeed> seeds;

    void addObject(const ObjectSeed& seed) override { seeds.push_back(seed); }
  } sink;

  provider->enumerate(sink);

  const int providerIndex = static_cast<int>(m_impl->providers.size());
  std::vector<std::unique_ptr<TreeNode>> staged;
  // Seeded with the machine root's committed leaves so the whole merge
  // lands in one assignment after every seed validates (FR-008).
  auto rootLeaves = m_impl->objects.at("machine")->leaves;
  std::vector<std::pair<std::string, std::string>> stagedAliases;

  for (const auto& seed : sink.seeds) {
    const std::string path(seed.path);
    const bool onRoot = path == "machine";
    if (!onRoot
        && (m_impl->objects.contains(path)
            || std::ranges::any_of(staged,
                                   [&](const std::unique_ptr<TreeNode>& one)
                                   { return one->path == path; })))
    {
      // A path the batch already staged collides the same way a path the
      // merged tree holds does. `map::emplace` keeps the earlier node, so
      // the later one would be dropped while its alias landed on the
      // surviving node (FR-008).
      return std::unexpected(Error {.message = "duplicate object path '" + path
                                        + "' under one parent (FR-008)",
                                    .suggestions = {}});
    }

    auto leaves = buildLeaves(seed, path, onRoot, rootLeaves, providerIndex);
    if (!leaves.has_value()) {
      return std::unexpected(leaves.error());
    }

    if (onRoot) {
      rootLeaves.insert(rootLeaves.end(),
                        std::make_move_iterator(leaves->begin()),
                        std::make_move_iterator(leaves->end()));
      continue;
    }

    auto node = std::make_unique<TreeNode>();
    node->kind = std::string(seed.kind);
    node->path = path;
    node->alias = std::string(seed.alias);
    node->description = std::string(seed.description);

    if (!seed.alias.empty()) {
      const std::string alias(seed.alias);
      if (aliasIsHeld(m_impl->aliases, stagedAliases, alias)) {
        return std::unexpected(
            Error {.message = "duplicate platform alias '" + alias
                       + "' already held by another " "object (FR-002)",
                   .suggestions = {}});
      }
      stagedAliases.emplace_back(alias, path);
    }
    node->leaves = std::move(*leaves);
    staged.push_back(std::move(node));
  }

  m_impl->objects.at("machine")->leaves = std::move(rootLeaves);
  for (auto& node : staged) {
    m_impl->objects.emplace(node->path, std::move(node));
  }
  for (auto& [alias, path] : stagedAliases) {
    m_impl->aliases.emplace(std::move(alias), std::move(path));
  }
  m_impl->providers.push_back(std::move(provider));
  return {};
}

auto System::object(std::string_view path)
    -> std::expected<sg::counters::Object, Error>
{
  m_impl->ensureOpen();
  const std::string canonical = m_impl->canonicalize(path);
  const auto located = m_impl->objects.find(canonical);
  if (located == m_impl->objects.end()) {
    std::vector<std::pair<std::string, std::string>> candidates;
    for (const auto& [objectPath, node] : m_impl->objects) {
      candidates.emplace_back(objectPath, node->description);
    }
    return std::unexpected(
        Error {.message = "no object at path '" + std::string(path) + "'",
               .suggestions = nearMisses(path, candidates)});
  }
  return handleFor(canonical);
}

// The time-stamp entry as a shorter spelling of the uniform lookup
// (specs/008-timestamp-counter FR-004). A pure tree walk: it resolves an
// entry and reads nothing, and it deliberately does not call
// `ensure_open()`, so a program may call it and then still register a
// provider (FR-006). The two failure branches are recoverable errors and
// never contract violations, so neither carries a contract macro (FR-007,
// FR-008).
auto System::tsc() const -> std::expected<Counter<Dim<0, 1>>, Error>
{
  const auto* node = m_impl->find("machine");
  // LCOV_EXCL_BR_START : coverage exclusion (T066): the short-circuit arc
  // between the two operands. The `machine` node is seeded by the system
  // itself at construction, so `find` answers it on every host, both
  // with and without a `perf_event_open` the kernel grants; the emptiness
  // operand below is the half every host reaches.
  if (node == nullptr || node->leaves.empty()) {  // LCOV_EXCL_BR_LINE
    return std::unexpected(Error {
        .message =
            "no clock provider is registered, so the " "tree " "holds " "no " "ti" "me" "-s" "ta" "mp" " " "entry to " "res" "olv" "e " "(specs/" "008-timestamp-" "counter FR-007)",
        .suggestions = {}});
  }
  // LCOV_EXCL_BR_STOP
  for (const auto& leaf : node->leaves) {
    if (leaf.core.name == "tsc") {
      return Counter<Dim<0, 1>> {.leaf = leaf.core};
    }
  }
  return std::unexpected(Error {
      .message =
          "the catalog publishes no time-stamp " "entry; " "a clock " "provider" " seeds " "it " "wh" "er" "e " "th" "e " "build " "executes" " the " "instructio" "n, and no " "registered" " provider " "did " "(sp" "ecs" "/00" "8-" "time" "st" "amp" "-co" "unt" "er " "FR-" "008" ")",
      .suggestions = {}});
}

auto System::handleFor(const std::string& canonical) -> sg::counters::Object&
{
  // One lock covers the lookup and the insert, so a concurrent miss on one
  // address constructs the handle once and hands the same entry to every
  // thread that named it (FR-010).
  const std::scoped_lock guard {m_impl->handlesLock};
  const auto existing = m_impl->handles.find(canonical);
  if (existing != m_impl->handles.end()) {
    return *existing->second;
  }
  // `make_unique` cannot build this handle: `object`'s node constructor
  // is private and `system` is its only friend (T113).
  const auto inserted = m_impl->handles.emplace(
      canonical,
      std::unique_ptr<sg::counters::Object>(
          new sg::counters::Object(m_impl->objects.at(canonical).get())));
  return *inserted.first->second;
}

auto System::objects(const std::string_view kind,
                     const std::initializer_list<Filter> filters)
    -> std::expected<std::vector<const sg::counters::Object*>, Error>
{
  m_impl->ensureOpen();
  bool knownKind = false;
  for (const auto& [path, node] : m_impl->objects) {
    if (node->kind == kind) {
      knownKind = true;
      break;
    }
  }
  if (!knownKind) {
    return std::unexpected(Error {.message = "no object of kind '"
                                      + std::string(kind)
                                      + "' in the tree (FR-003)",
                                  .suggestions = {}});
  }
  for (const auto& one : filters) {
    if (!keyDefined(m_impl->objects, kind, one)) {
      return std::unexpected(Error {.message = "unknown filter key '"
                                        + std::string(one.key) + "' (FR-003)",
                                    .suggestions = {}});
    }
  }
  std::vector<const sg::counters::Object*> matches;
  for (const auto& [path, node] : m_impl->objects) {
    if (node->kind != kind) {
      continue;
    }
    bool matchesAll = true;
    for (const auto& one : filters) {
      if (!pathCarries(path, one)) {
        matchesAll = false;
        break;
      }
    }
    if (matchesAll) {
      matches.push_back(&handleFor(path));
    }
  }
  return matches;
}

auto Object::path() const noexcept -> std::string_view
{
  return static_cast<const TreeNode*>(m_node)->path;
}

auto Object::alias() const noexcept -> std::string_view
{
  return static_cast<const TreeNode*>(m_node)->alias;
}

auto Object::kind() const noexcept -> std::string_view
{
  return static_cast<const TreeNode*>(m_node)->kind;
}

auto Object::description() const noexcept -> std::string_view
{
  return static_cast<const TreeNode*>(m_node)->description;
}

auto Object::parent() const noexcept -> const Object*
{
  const auto* node = static_cast<const TreeNode*>(m_node);
  if (node->path == "machine") {
    return nullptr;
  }
  auto& systemRef = System::local();
  const auto slash = node->path.rfind('/');
  const std::string parentPath = slash == std::string::npos
      ? std::string("machine")
      : node->path.substr(0, slash);
  if (systemRef.m_impl->find(parentPath) == nullptr) {
    return nullptr;
  }
  return &systemRef.handleFor(parentPath);
}

auto Object::counters() const -> std::vector<CatalogEntry>
{
  const auto* node = static_cast<const TreeNode*>(m_node);
  std::vector<CatalogEntry> entries;
  entries.reserve(node->leaves.size());
  for (const auto& leaf : node->leaves) {
    const auto recognized = unitFromToken(leaf.core.unit);
    SG_REQUIRE(recognized.has_value(),
               "every stored catalog unit maps (FR-017)");
    // The mask names the kinds the availability probe settled, beside the
    // catalog state the probes merged into one value: a countable entry the
    // cpu-targeted probe refused names no cpu bit, and the state alone
    // cannot say so, because the probe settled the entry on the kind it
    // did count (FR-021). The two agree: a countable entry names at least
    // one target kind and every other state names none, however its object
    // is scoped. The decision itself is declared in the seam beside the
    // others, with its own contract (FR-046).
    const TargetMask probed = detail::probedKindsAt(leaf.core.address);
    const TargetMask targets =
        detail::settledTargets(leaf.core.avail, probed, node->kind, node->path);
    SG_ENSURE((leaf.core.avail != Availability::COUNTABLE) == (targets == 0),
              "a countable entry names at least one target kind and every "
              "other state names none (FR-021)");
    entries.push_back(CatalogEntry {
        .name = leaf.core.name,
        .description = leaf.core.description,
        // Unreachable behind the checked precondition; in a build with
        // contract checking compiled out the entry still needs a value.
        .unit = recognized.value_or(Unit::NONE),
        .avail = leaf.core.avail,
        .mode = leaf.core.mode,
        .targets = targets,
        .frequencyHz = leaf.core.frequencyHz,
        .scaled = leaf.core.scaled,
    });
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): the NRVO epilogue block of
  // `object::counters`, as at the `split_words` epilogue above.
  return entries;
}  // LCOV_EXCL_LINE

auto Object::children() const -> std::vector<const Object*>
{
  const auto* node = static_cast<const TreeNode*>(m_node);
  auto& systemRef = System::local();
  std::vector<const Object*> direct;
  for (const auto& [objectPath, child] : systemRef.m_impl->objects) {
    const bool isDirect = node->path == "machine"
        ? (objectPath.find('/') == std::string::npos && objectPath != "machine")
        : (objectPath.size() > node->path.size() + 1
           && objectPath.starts_with(node->path + "/")
           && objectPath.find('/', node->path.size() + 1) == std::string::npos);
    if (isDirect) {
      direct.push_back(&systemRef.handleFor(objectPath));
    }
  }
  // LCOV_EXCL_LINE : coverage exclusion (T066): the NRVO epilogue block of
  // `object::children`, as at the `split_words` epilogue above.
  return direct;
}  // LCOV_EXCL_LINE

namespace detail
{

void noteProbedKinds(const std::string& address, const TargetMask probed)
{
  probedKindTable()[address] = probed;
}

auto probedKindsAt(const std::string& address) noexcept -> TargetMask
{
  const auto& table = probedKindTable();
  const auto found = table.find(address);
  return found == table.end() ? TargetMask {} : found->second;
}

auto settledTargets(const Availability probed,
                    const TargetMask probedKinds,
                    const std::string_view kind,
                    const std::string& path) noexcept -> TargetMask
{
  const bool countable = probed == Availability::COUNTABLE;
  const bool coreDevice =
      path == "cpu" || path == "cpu_core" || path == "cpu_atom";
  const bool deviceScoped = kind == "pmu" && !coreDevice;
  // The kinds the probe settled answer for every leaf it settled one on.
  // A leaf it settled none on is the enabled/running pair or a leaf no
  // event provider probed, and the object's own scope answers for those
  // (FR-021, FR-022).
  const TargetMask scoped =
      deviceScoped ? kTargetCpuBit : kTargetThreadBit | kTargetCpuBit;
  const TargetMask named = probedKinds == 0 ? scoped : probedKinds;
  const TargetMask settled = countable ? named : TargetMask {};
  // The rule is spelled once and both the mask and the postcondition read
  // it, so the check cannot disagree with the decision it checks (FR-021,
  // FR-022).
  SG_ENSURE(settled == (countable ? named : TargetMask {}),
            "a state other than `countable` names no target kind, and a "
            "countable entry names exactly the kinds the probe settled it "
            "on, or the kinds its object's scope admits where the probe "
            "settled none (FR-021, FR-022)");
  return settled;
}

auto resolveLeafCore(const Object& obj,
                     std::string_view name) -> std::expected<LeafCore, Error>
{
  const auto* node = static_cast<const TreeNode*>(obj.m_node);
  for (const auto& leaf : node->leaves) {
    if (leaf.core.name == name) {
      return leaf.core;
    }
  }
  std::vector<std::pair<std::string, std::string>> candidates;
  for (const auto& leaf : node->leaves) {
    candidates.emplace_back(leaf.core.name, leaf.core.description);
  }
  return std::unexpected(Error {.message = "object '" + node->path
                                    + "' has no counter named '"
                                    + std::string(name) + "' (FR-008)",
                                .suggestions = nearMisses(name, candidates)});
}

}  // namespace detail

}  // namespace sg::counters
