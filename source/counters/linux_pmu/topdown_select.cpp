// ============================================================================
// The identity a top-down selection reads, the table it selects and the tree it
// answers (specs/042-topdown T022, T024; FR-001 to FR-003, FR-008 to FR-016,
// FR-020; data-model E-01 to E-09).
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
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

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

// The source record of the directory a source already names, filled in place.
// The reference is the one text the manifest states for the whole vendored
// tree, and the files are the entries of that directory's index that hold a
// row of the parsed table, each with the digest the manifest line beside its
// name states. The embedder emitted the index in a sorted order and the loader
// reads the directory in it, so the entries reach the record in the embedder's
// order and a file that holds no row reaches no entry (FR-015, FR-033,
// spec D-12, data-model E-02, E-03).
auto fillSourceRecord(TopdownSource& source) -> void
{
  // A directory arrives with a trailing separator and the registry names it
  // without one, which is the spelling the table loader reads (FR-036).
  std::string_view key {source.directory};
  if (key.ends_with('/')) {
    key.remove_suffix(1);
  }
  const detail::EmbeddedDir* record = detail::embeddedFindDir(key);
  // LCOV_EXCL_BR_START : coverage exclusion (T101): the arm where the registry
  // holds no bytes for a directory the mapping file named. The generator
  // embeds every directory of the vendored tree, naming each file its glob
  // finds and refusing a file the manifest names no digest for, and the
  // mapping file names only directories of that tree, so a selected directory
  // stands in the registry of every archive this library links (FR-036).
  if (record == nullptr) {
    return;
  }
  // LCOV_EXCL_BR_STOP
  source.reference = record->reference;
  const std::vector<detail::PmuTableEntry>& table =
      detail::pmuLoadTable(source.directory);
  for (std::size_t index = 0; index != record->fileCount; ++index) {
    // The registry's index is a generated C array and its count bounds this
    // loop, so the subscript is in range by construction.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const detail::EmbeddedFile& file = record->files[index];
    const std::string_view name {file.name};
    const bool contributed =
        std::ranges::any_of(table,
                            [name](const detail::PmuTableEntry& entry)
                            { return entry.fileName == name; });
    if (contributed) {
      source.files.push_back(
          TableFile {.name = std::string {name},
                     .manifestHash = std::string {file.sha256}});
    }
  }
}

}  // namespace
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
    fillSourceRecord(table.source);
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

  // The level each row's group text states, the deepest level the table holds,
  // and the first row of each name: the first row of a duplicated name is the
  // row the directory answers, which is the rule the metric layer's own index
  // follows (FR-011, FR-013, FR-014).
  std::vector<int> levels;
  levels.reserve(table.rows.size());
  std::unordered_map<std::string, std::size_t> rowsByName;
  int deepest = 0;
  for (std::size_t index = 0; index != table.rows.size(); ++index) {
    int level = 0;
    for (const std::string_view token : tokensOf(table.rows[index].group)) {
      const int stated = levelOfToken(token);
      level = stated > level ? stated : level;
    }
    levels.push_back(level);
    deepest = level > deepest ? level : deepest;
    rowsByName.insert({table.rows[index].name, index});
  }
  const int depth = options.level < deepest ? options.level : deepest;

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

  // The shape the contract states, read back over the answer: a node names a
  // row the table published, stands at or above the depth the answer states,
  // and names a parent that stands earlier in the vector (FR-008, FR-013,
  // FR-020).
  [[maybe_unused]] const bool shapeHolds =
      [&table, &rowsByName](const TopdownTree& built) -> bool
  {
    for (std::size_t here = 0; here != built.nodes.size(); ++here) {
      const TopdownNode& node = built.nodes[here];
      const auto stated = rowsByName.find(node.vendorName);
      if (stated == rowsByName.end() || !table.rows[stated->second].published) {
        return false;
      }
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
  }(tree);

  SG_ENSURE(tree.depth == options.level || tree.depth == deepest,
            "the depth is the asked level clamped to the deepest level the "
            "table " "holds (FR-008, FR-020)");
  SG_ENSURE(shapeHolds,
            "no node stands that a published row of the table does not state, "
            "every parent names an element that stands earlier in the vector, "
            "and that element stands one level above (FR-013)");
  return tree;
}

}  // namespace sg::counters
