// ============================================================================
// The identity a top-down selection reads, the table it selects, the tree it
// answers and the refusals it answers instead (specs/042-topdown T022, T024,
// T079; FR-001 to FR-007, FR-008 to FR-016, FR-020, FR-040; data-model E-01 to
// E-11).
//
// Three calls live here, each over a seam of the private pmu header. The
// identity copies the four fields the library's own cpu reader answers and
// leaves the two fields no reader answers absent, because a reader for the
// core type of a hybrid part or for an Arm `MIDR` stands nowhere in this
// feature (spec D-13). The listing hands the identity's four production fields
// to the same mapping-file selection the catalog uses, so an answer about
// another part comes out of the identity the caller handed in and never out of
// the cpu this process runs on (FR-002). The tree reads the rows of one listing
// beside the group text each row carries.
//
// Both of the first two calls read the bytes this build embedded: the mapping
// file is the arch/x86 directory's own file there, so the selection opens no
// path (specs/012 FR-036, spec D-12). The rows of a directory and their
// verdicts reach a listing through the metric layer, which resolves a row
// against the leaves of the open system once per directory. The reference of
// the vendored tree and the digest of every file that contributed a row reach
// it through the record the embedder put beside that directory's bytes, so the
// source record reads no path either (FR-015, spec D-12). The listing reads
// both settled answers as data and opens no system of its own, so no call here
// opens an event, samples a counter, reads a clock or calls a cpu reader
// (FR-002, FR-009).
//
// The tree pass reads the rows it is handed. It splits each row's group text
// into the two shapes the vendor tables state: the level token FR-014 names,
// and the parent token FR-013 names, spelled `tma_<name>_group` by Intel and
// `<name>_group` by AMD. A published row of a level the ask reaches, with a
// parent the tree already holds, becomes one node. A refused row of such a
// level becomes one entry of `refused` and no node (FR-016).
//
// Two of the five refusals read those same rows ahead of the tree: a directory
// whose rows carry no level token answers NO_TOPDOWN_ROWS, and a directory
// whose event rows carry the two core scopes answers HYBRID. The first stands
// ahead of the second, because a directory with two scopes and no level row
// states no tree. The three that read the identity answer ahead of both.
// `message()` owns the one line each of the five answers, one sentence per
// kind, and no other part of this band composes that sentence (FR-004 to
// FR-007, FR-040, spec D-14).
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "topdown_select.hpp"

#include "../detail/pmu.hpp"
#include "embedded_tables.hpp"
#include "metric_formula.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/counters_topdown.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{

#ifdef __linux__
namespace
{

// One row of the metric layer's answer as the listing carries it: the texts of
// the table, the spine the layer built for the row, and the verdict the layer
// answered with (FR-010, FR-016, FR-017).
[[nodiscard]] auto listedRow(const detail::MetricDefinition& def) -> TopdownRow
{
  TopdownRow row;
  row.name = def.name;
  row.group = def.group;
  row.unit = def.unit;
  row.threshold = def.threshold;
  row.formula.core = def.core;
  row.published = def.published;
  row.refusal = def.refusal;
  row.reason = def.reason;
  return row;
}

}  // namespace

namespace detail
{

auto fillSourceRecord(TopdownSource& source) -> void
{
  // A directory arrives with a trailing separator and the registry names it
  // without one, which is the spelling the table loader reads (FR-036).
  std::string_view key {source.directory};
  if (key.ends_with('/')) {
    key.remove_suffix(1);
  }
  const EmbeddedDir* record = embeddedFindDir(key);
  // A name the registry holds no bytes for answers the source as it stands: no
  // reference, no file. The mapping file names only directories of the vendored
  // tree, so every directory a host selects stands in this registry, and the
  // test for this arm hands in a name no mapping file carries (FR-036).
  if (record == nullptr) {
    return;
  }
  source.reference = record->reference;
  const std::vector<PmuTableEntry>& table = pmuLoadTable(source.directory);
  for (std::size_t index = 0; index != record->fileCount; ++index) {
    // The registry's index is a generated C array and its count bounds this
    // loop, so the subscript is in range by construction.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const EmbeddedFile& file = record->files[index];
    const std::string_view name {file.name};
    const bool contributed = std::ranges::any_of(
        table,
        [name](const PmuTableEntry& entry) { return entry.fileName == name; });
    if (contributed) {
      source.files.push_back(
          TableFile {.name = std::string {name},
                     .manifestHash = std::string {file.sha256}});
    }
  }
}

}  // namespace detail
#endif  // __linux__

auto cpuIdentity() -> CpuIdentity
{
#ifdef __linux__
  const detail::PmuIdent read = detail::pmuIdentCurrent();
  CpuIdentity identity;
  identity.vendor = read.vendor;
  identity.family = read.family;
  identity.model = read.model;
  identity.stepping = read.stepping;
  SG_ENSURE(identity.vendor == read.vendor && identity.family == read.family
                && identity.model == read.model
                && identity.stepping == read.stepping
                && identity.coreType.empty() && !identity.midr.has_value(),
            "the four fields carry the reader's own answer and the two fields "
            "no reader answers stand absent (FR-003, spec D-13)");
  return identity;
#else
  // The reduced catalog of a platform that publishes no event table carries no
  // cpu to identify, so the identity names an unreadable cpu: no vendor text,
  // no family, no model and no stepping class (specs/007 FR-042).
  return {};
#endif
}

auto topdownTable(const CpuIdentity& identity) -> TopdownTable
{
  TopdownTable table;
#ifdef __linux__
  // The mapping file matches the vendor, the family, the model and the
  // stepping, and no row of it reads a core type or an `MIDR`, so the four
  // fields build the whole key (spec D-13).
  const detail::PmuIdent ident {
      .vendor = identity.vendor,
      .family = identity.family,
      .model = identity.model,
      .stepping = identity.stepping,
  };
  table.source.directory = detail::pmuSelectDirectory(ident);
  if (!table.source.directory.empty()) {
    // The reference and the digests the reader of a table owes come out of the
    // record the embedder put beside this directory's bytes, so the fill reads
    // no path and opens nothing: the manifest texts are already in the archive
    // (FR-015, spec D-12).
    detail::fillSourceRecord(table.source);
    // The verdicts of the rows come from the metric layer's own pass over the
    // parsed rows of this directory, which resolves once per directory against
    // the system this process already holds. The host carries the directory
    // this identity selects, so the listing follows the identity handed in, and
    // the pass reads settled data: no event open, no counter sample and no cpu
    // reader stand in it (FR-001, FR-002, FR-016).
    const detail::MetricHost host {
        .system = &System::local(),
        .directory = table.source.directory,
    };
    for (const detail::MetricDefinition& def :
         detail::resolvedMetricTable(host).rows)
    {
      table.rows.push_back(listedRow(def));
    }
  }
#endif
  SG_ENSURE(!table.source.directory.empty() || table.rows.empty(),
            "an identity the mapping file matched no row for names no "
            "directory and answers no row (FR-001)");
  return table;
}

auto topdownEventGroups(const CpuIdentity& identity)
    -> std::vector<TopdownEventGroup>
{
  std::vector<TopdownEventGroup> pairs;
#ifdef __linux__
  // The mapping file matches the vendor, the family, the model and the
  // stepping, and no row of it reads a core type or an `MIDR`, so the four
  // fields build the whole key. The directory is all of the identity this call
  // reads (spec D-13, D-16).
  const detail::PmuIdent ident {
      .vendor = identity.vendor,
      .family = identity.family,
      .model = identity.model,
      .stepping = identity.stepping,
  };
  const std::string directory = detail::pmuSelectDirectory(ident);
  if (!directory.empty()) {
    // The rows reach the call as the data the loader holds once per directory,
    // so the listing opens no path, opens no event, samples no counter and
    // reads no clock (D-16). Every row reaches one pair, event row and metric
    // row alike, and its group is the stem of the file that row stands in: the
    // vendor's own word for the group, with nothing renamed (FR-004).
    const std::vector<detail::PmuTableEntry>& rows =
        detail::pmuLoadTable(directory);
    std::unordered_set<std::string> held;
    for (const detail::PmuTableEntry& row : rows) {
      // The first row that carries a name owns the pair of that name, so one
      // insert-if-absent line is the whole of the rule and a caller reads no
      // dedup rule of its own (D-16).
      if (held.insert(row.name).second) {
        pairs.push_back(TopdownEventGroup {
            .event = row.name,
            .group = std::filesystem::path {row.fileName}.stem().string(),
        });
      }
    }
  }
#endif
  // The check reads the listing the call built and nothing else: it names the
  // pair rule of a repeated name, and it names no fact about the rows of one
  // vendor's file (D-16, ruling R3).
  std::unordered_set<std::string> distinct;
  for (const TopdownEventGroup& pair : pairs) {
    distinct.insert(pair.event);
  }
  SG_ENSURE(distinct.size() == pairs.size(),
            "the answer holds one pair per name of the selected directory, so a "
            "name two rows carry reaches one pair, owned by the row that came "
            "first (FR-004, D-16)");
  return pairs;
}

namespace
{

// The vendor texts of the two vendors whose tables this build embeds (FR-004,
// FR-012).
constexpr std::string_view kIntelVendor {"GenuineIntel"};
constexpr std::string_view kAmdVendor {"AuthenticAMD"};

// The one level-1 name an AMD table answers alone (FR-011, spec D-3).
constexpr std::string_view kSmtContention {"smt_contention"};

// The row index that names no row.
constexpr std::size_t kNoRow {std::numeric_limits<std::size_t>::max()};

// The tokens one group text holds, cut on the `;` the tables spell (FR-013,
// FR-014).
[[nodiscard]] auto tokensOf(const std::string& group)
    -> std::vector<std::string_view>
{
  const std::string_view text {group};
  std::vector<std::string_view> tokens;
  std::size_t from = 0;
  while (true) {
    const std::size_t cut = text.find(';', from);
    if (cut == std::string_view::npos) {
      tokens.push_back(text.substr(from));
      return tokens;
    }
    tokens.push_back(text.substr(from, cut - from));
    from = cut + 1;
  }
}

// The level one group token states, and 0 for a token that names no level.
// Intel spells the token `TopdownL<n>` with `n` in 1 to 6, and AMD spells
// `PipelineL<n>` with `n` in 1 and 2 (FR-014, spec D-2).
[[nodiscard]] auto levelOfToken(const std::string_view token) -> int
{
  static constexpr std::array<std::pair<std::string_view, int>, 2> kTokens {
      {{"TopdownL", 6}, {"PipelineL", 2}}};
  for (const auto& [prefix, deepest] : kTokens) {
    if (token.size() != prefix.size() + 1 || !token.starts_with(prefix)) {
      continue;
    }
    const char digit = token[prefix.size()];
    if (digit >= '1' && digit <= static_cast<char>('0' + deepest)) {
      return digit - '0';
    }
  }
  return 0;
}

// The row a group text names as its parent: the row that the stem of one of its
// `_group` tokens names, and `kNoRow` where no token stems to another row of
// the table. A root carries `tma_L1_group`, whose stem names no row, so the
// answer for a root is `kNoRow` (FR-013).
[[nodiscard]] auto parentRowOf(
    const TopdownRow& row,
    const std::size_t own,
    const std::unordered_map<std::string, std::size_t>& rowsByName)
    -> std::size_t
{
  static constexpr std::string_view kSuffix {"_group"};
  for (const std::string_view token : tokensOf(row.group)) {
    if (token.size() <= kSuffix.size() || !token.ends_with(kSuffix)) {
      continue;
    }
    const std::string stem {token.substr(0, token.size() - kSuffix.size())};
    const auto named = rowsByName.find(stem);
    if (named != rowsByName.end() && named->second != own) {
      return named->second;
    }
  }
  return kNoRow;
}

// The index of the first row of each name of the table: the first row of a
// duplicated name is the row the directory answers, which is the rule the
// metric layer's own index follows (FR-011).
[[nodiscard]] auto firstRowOfName(const TopdownTable& table)
    -> std::unordered_map<std::string, std::size_t>
{
  std::unordered_map<std::string, std::size_t> rowsByName;
  for (std::size_t index = 0; index != table.rows.size(); ++index) {
    rowsByName.insert({table.rows[index].name, index});
  }
  return rowsByName;
}

// The name a level-1 row states once its vendor's own prefix stands aside.
// Intel spells a level-1 row `tma_<shared name>` and AMD spells the shared name
// bare, so the prefix is the one text a neutral name drops (FR-011, spec D-2).
[[nodiscard]] auto sharedNameOf(const std::string_view name) -> std::string_view
{
  static constexpr std::string_view kPrefix {"tma_"};
  return name.starts_with(kPrefix) ? name.substr(kPrefix.size()) : name;
}

// The vendor-neutral name of one row: the shared name of FR-011 at level 1, and
// the vendor's own name behind a vendor prefix below it (FR-012, spec D-14).
[[nodiscard]] auto neutralNameOf(std::string_view name,
                                 const int level,
                                 const bool amd) -> std::string
{
  if (level == 1) {
    return "topdown." + std::string {sharedNameOf(name)};
  }
  return "topdown." + std::string {amd ? "amd" : "intel"} + '.'
      + std::string {name};
}

// The identity in the spelling an unsupported answer carries: the family and
// the model, and the stepping where one stands (FR-040, spec D-14).
[[nodiscard]] auto identityText(const CpuIdentity& identity) -> std::string
{
  std::string text {"family " + std::to_string(identity.family) + " model "
                    + std::to_string(identity.model)};
  if (identity.stepping >= 0) {
    text += " stepping " + std::to_string(identity.stepping);
  }
  return text;
}

// The `MIDR` of an Arm part in the hexadecimal spelling such a part publishes
// (FR-040, spec D-13).
[[nodiscard]] auto midrText(const std::uint32_t midr) -> std::string
{
  static constexpr std::string_view kDigits {"0123456789abcdef"};
  std::string text {"0x"};
  for (int shift = 28; shift >= 0; shift -= 4) {
    text += kDigits[(midr >> static_cast<unsigned>(shift)) & 0xFU];
  }
  return text;
}

// The two core scopes a hybrid part's directory publishes, spelled the way a
// hybrid table spells them in the `Unit` text of its own rows (FR-006).
constexpr std::string_view kCoreScope {"cpu_core"};
constexpr std::string_view kAtomScope {"cpu_atom"};

// Whether the event rows of `directory` publish both core scopes: the fact
// FR-006 reads, held in the `Unit` text of a row of the directory's own parsed
// table (`PmuTableEntry::unit`). A scope of another class, the memory class of
// an uncore row among them, counts for nothing here, and a directory whose rows
// carry no `Unit` key at all publishes one core scope. The load is the
// once-per-directory read the listing already reached, so this call opens no
// path and reads no counter (FR-006, specs/012 FR-036).
[[nodiscard]] auto publishesTwoScopes(const std::string& directory) -> bool
{
#ifdef __linux__
  bool core = false;
  bool atom = false;
  for (const detail::PmuTableEntry& entry : detail::pmuLoadTable(directory)) {
    core = core || entry.unit == kCoreScope;
    atom = atom || entry.unit == kAtomScope;
  }
  return core && atom;
#else
  // A platform that publishes no event table holds no directory to read, so no
  // directory of it publishes two scopes (specs/007 FR-042).
  static_cast<void>(directory);
  return false;
#endif
}

// A refusal, with the kind and the three fields the identity states. The line a
// caller prints is `message()`'s, so no text here carries a sentence
// (FR-040, spec D-14).
[[nodiscard]] auto refusedWith(TopdownUnsupport kind,
                               std::string vendor,
                               std::string identity,
                               std::string detail) -> TopdownUnsupported
{
  return TopdownUnsupported {
      .kind = kind,
      .vendor = std::move(vendor),
      .identity = std::move(identity),
      .detail = std::move(detail),
  };
}

}  // namespace

namespace detail
{

auto topdownShapeHolds(const TopdownTree& built) -> bool
{
  for (std::size_t here = 0; here != built.nodes.size(); ++here) {
    const TopdownNode& node = built.nodes[here];
    if (node.level < 1 || node.level > built.depth) {
      return false;
    }
    if (node.parent.empty() == (node.level != 1)) {
      return false;
    }
    if (node.parent.empty()) {
      continue;
    }
    const TopdownNode* upstream = nullptr;
    for (std::size_t earlier = 0; earlier != here; ++earlier) {
      if (built.nodes[earlier].name == node.parent) {
        upstream = &built.nodes[earlier];
        break;
      }
    }
    if (upstream == nullptr || upstream->level + 1 != node.level) {
      return false;
    }
  }
  return true;
}

auto topdownPublishedHolds(const TopdownTree& built,
                           const TopdownTable& table) -> bool
{
  const std::unordered_map<std::string, std::size_t> rowsByName =
      firstRowOfName(table);
  return std::ranges::all_of(built.nodes,
                             [&table, &rowsByName](const TopdownNode& node)
                             {
                               const auto stated =
                                   rowsByName.find(node.vendorName);
                               return stated != rowsByName.end()
                                   && table.rows[stated->second].published;
                             });
}

}  // namespace detail

auto TopdownUnsupported::message() const -> std::string
{
  // The reason of the kind, spelled once here. The two refusals that read no
  // directory state their reason in full, the two a table's own rows state
  // carry that directory between quote marks, and an `MIDR` carries the
  // sentence its refusal states (FR-004 to FR-007, spec D-14).
  std::string reason;
  switch (kind) {
    case TopdownUnsupport::UNKNOWN_VENDOR:
      reason = "no vendored table carries this vendor";
      break;
    case TopdownUnsupport::NO_DIRECTORY:
      reason = "the mapping file names no table for this identity";
      break;
    case TopdownUnsupport::HYBRID:
      reason = "the selected table '" + detail
          + "' publishes rows for two core scopes, and this feature makes no "
            "claim for a hybrid part";
      break;
    case TopdownUnsupport::ARM:
      reason = detail;
      break;
    case TopdownUnsupport::NO_TOPDOWN_ROWS:
      reason = "the selected table '" + detail
          + "' holds no row of any top-down group";
      break;
  }
  const std::string line =
      "topdown: " + vendor + ' ' + identity + ": " + reason;
  SG_ENSURE(line.starts_with("topdown: ") && !line.contains('\n')
                && line.contains(vendor) && line.contains(identity)
                && line.contains(reason),
            "the refusal answers one line that names the vendor, the identity "
            "and the reason of its kind (FR-040, spec D-14)");
  return line;
}

auto topdownTree(const TopdownSelection& options)
    -> std::expected<TopdownTree, TopdownUnsupported>
{
  SG_REQUIRE(options.level >= 1 && options.level <= 6,
             "a selection asks one of the levels the level tokens of the "
             "tables " "carry, which are 1 to 6 (FR-018, FR-019)");
  const CpuIdentity& identity = options.identity;
  if (identity.midr.has_value()) {
    // An `MIDR` names an Arm part and this build vendors no Arm table, so the
    // word the identity carries is the whole identity such a part states
    // (FR-007, spec D-13).
    return std::unexpected(refusedWith(TopdownUnsupport::ARM,
                                       "Arm",
                                       midrText(*identity.midr),
                                       "no Arm table is vendored"));
  }
  const bool amd = identity.vendor == kAmdVendor;
  if (!amd && identity.vendor != kIntelVendor) {
    // The neutral name of a node below level 1 carries the vendor, so a vendor
    // of no vendored table answers no tree (FR-004, FR-012).
    return std::unexpected(refusedWith(TopdownUnsupport::UNKNOWN_VENDOR,
                                       identity.vendor,
                                       identityText(identity),
                                       {}));
  }
  const TopdownTable table =
      options.table.has_value() ? *options.table : topdownTable(identity);
  if (table.source.directory.empty()) {
    // The mapping file matched no row of its own for these four fields
    // (FR-005).
    return std::unexpected(refusedWith(TopdownUnsupport::NO_DIRECTORY,
                                       identity.vendor,
                                       identityText(identity),
                                       {}));
  }

  // The level each row's group text states, and the deepest level the table
  // holds (FR-013, FR-014).
  std::vector<int> levels;
  levels.reserve(table.rows.size());
  int deepest = 0;
  for (std::size_t index = 0; index != table.rows.size(); ++index) {
    int level = 0;
    for (const std::string_view token : tokensOf(table.rows[index].group)) {
      const int stated = levelOfToken(token);
      level = stated > level ? stated : level;
    }
    levels.push_back(level);
    deepest = level > deepest ? level : deepest;
  }
  // The first row of each name, the index the parent search of the pass reads
  // (FR-011).
  const std::unordered_map<std::string, std::size_t> rowsByName =
      firstRowOfName(table);
  const int depth = options.level < deepest ? options.level : deepest;

  // Two refusals read the rows of the selected directory, and they stand ahead
  // of any tree. A directory whose rows carry no level token holds no top-down
  // row at all, and that answer stands first: a directory that publishes the
  // two core scopes beside no level row states no tree to refuse, which is the
  // shape one vendored directory answers with 606 event rows and no metric row
  // (FR-006, FR-007, spec D-14).
  if (deepest == 0) {
    return std::unexpected(refusedWith(TopdownUnsupport::NO_TOPDOWN_ROWS,
                                       identity.vendor,
                                       identityText(identity),
                                       table.source.directory));
  }
  // The two core scopes are the fact FR-006 refuses on, and the rule reads it
  // from the directory's own event rows: the next hybrid directory is a hybrid
  // directory at its first atom row, so a list of directory names would name it
  // one release late.
  if (publishesTwoScopes(table.source.directory)) {
    return std::unexpected(refusedWith(TopdownUnsupport::HYBRID,
                                       identity.vendor,
                                       identityText(identity),
                                       table.source.directory));
  }

  TopdownTree tree;
  tree.source = table.source;
  tree.depth = depth;
  tree.amd = amd;

  // One node per published row of a level the answer reaches, level by level,
  // so every parent names an element that stands earlier in the vector. A row
  // whose parent token stems to no row, or to a row the tree holds no node for,
  // stands out of the tree, because no part of this pass invents a parent
  // (FR-013). A table may name a parent at its own row's level, as the memory
  // rows of an Intel directory do, and that parent stands ahead of its child in
  // table order, so both rows reach the vector (FR-016, spec D-20).
  std::unordered_map<std::size_t, std::string> neutralOf;
  for (int level = 1; level <= depth; ++level) {
    for (std::size_t index = 0; index != table.rows.size(); ++index) {
      const TopdownRow& row = table.rows[index];
      if (levels[index] != level || !row.published) {
        continue;
      }
      if (level == 1 && !amd && sharedNameOf(row.name) == kSmtContention) {
        // The fifth value belongs to an AMD table alone (FR-011, spec D-3).
        continue;
      }
      std::string parent;
      if (level > 1) {
        const std::size_t above = parentRowOf(row, index, rowsByName);
        const auto built = neutralOf.find(above);
        if (above == kNoRow || built == neutralOf.end()) {
          continue;
        }
        parent = built->second;
      }
      TopdownNode node;
      node.name = neutralNameOf(row.name, level, amd);
      node.vendorName = row.name;
      node.parent = std::move(parent);
      node.level = level;
      node.formula = row.formula;
      for (const detail::LeafCore& leaf : row.formula.core.leaves) {
        node.events.push_back(leaf.address);
      }
      node.threshold = row.threshold;
      neutralOf.insert({index, node.name});
      tree.nodes.push_back(std::move(node));
    }
  }

  // A refused row stands in the document at the level its own token states,
  // with the kind and the reason the metric layer answered with, and it builds
  // no node (FR-016, FR-031).
  for (std::size_t index = 0; index != table.rows.size(); ++index) {
    const TopdownRow& row = table.rows[index];
    if (row.published || levels[index] == 0 || levels[index] > depth) {
      continue;
    }
    TopdownRefusal entry;
    entry.name = row.name;
    entry.level = levels[index];
    entry.refusal = row.refusal;
    entry.reason = row.reason;
    const std::size_t above = parentRowOf(row, index, rowsByName);
    if (above != kNoRow) {
      const int aboveLevel =
          levels[above] > 0 ? levels[above] : levels[index] - 1;
      entry.parent = neutralNameOf(table.rows[above].name, aboveLevel, amd);
    }
    tree.refused.push_back(std::move(entry));
  }

  // The two readbacks of the postcondition below, over the answer the pass
  // built. Each states one clause of it, and the contract of each stands in
  // `topdown_select.hpp` (FR-008, FR-011, FR-013, FR-016, FR-020).
  [[maybe_unused]] const bool shapeHolds = detail::topdownShapeHolds(tree);
  [[maybe_unused]] const bool publishedHolds =
      detail::topdownPublishedHolds(tree, table);

  SG_ENSURE(tree.depth == options.level || tree.depth == deepest,
            "the depth is the asked level clamped to the deepest level the "
            "table " "holds (FR-008, FR-020)");
  SG_ENSURE(shapeHolds && publishedHolds,
            "no node stands that a published row of the table does not state, "
            "every parent names an element that stands earlier in the vector, "
            "and that element stands one level above (FR-013)");
  return tree;
}

}  // namespace sg::counters
