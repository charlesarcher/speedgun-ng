// ============================================================================
// Vendor-neutral top-down analysis over the vendored event table
// (specs/042-topdown; FR-001 to FR-003, FR-010 to FR-017, FR-040; data-model
// E-01 to E-11).
//
// One cpu identity selects one architecture table directory, and the rows of
// that directory's top-down groups form the tree a run collects. This header
// publishes the identity, the source record of the selected table, the tree,
// and the one refusal a selection answers.
//
// The selection is pure over what it is handed: it reads no counter, opens no
// event, reads no clock and calls no cpu reader of its own, so its answer
// follows from the identity it is given and the rows it is given (FR-002).
// The mapping file, the table bytes and the manifest digests reach it as data
// this build embedded, so the call opens no path (specs/012 FR-036, spec
// D-12).
//
// The report writer owns the console block, the JSON keys and the record. This
// header holds no text for any of them. A refused row reaches the reader as a
// row of its own, with the metric layer's kind and reason beside it (FR-016).
// ============================================================================

#ifndef SG_COUNTERS_TOPDOWN_HPP
#define SG_COUNTERS_TOPDOWN_HPP

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_metrics.hpp"

namespace sg::counters
{

/**
 * @brief The cpu identity a top-down selection reads (specs/042-topdown
 * FR-003, spec D-13, data-model E-01).
 *
 * `vendor` is the vendor text CPUID leaf 0 answers, twelve characters, and an
 * empty text names an unreadable cpu. `family`, `model` and `stepping` carry
 * the mapping file's own spelling, decimal family and hexadecimal model, and
 * a negative `stepping` names no stepping class. That digit is what splits
 * one model name into two vendor directories, so the identity carries it
 * beside the model.
 *
 * `coreType` names the core class of a hybrid part and `midr` the `MIDR` of
 * an Arm part. Both stand absent on every production host of this feature: no
 * reader for either exists here, and a test hands both in (spec D-13).
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT CpuIdentity
{
  std::string vendor;  ///< CPUID leaf 0, twelve characters.
  int family = -1;  ///< The mapping file's decimal spelling.
  int model = -1;  ///< The mapping file's hexadecimal spelling.
  int stepping = -1;  ///< Negative names no stepping class.
  std::string coreType {};  ///< Empty in production (spec D-13).
  std::optional<std::uint32_t> midr {};  ///< Unset in production (spec D-13).
};

/// @brief The identity of the cpu this process runs on.
///
/// The four fields the library already reads come from its own reader, and
/// the two fields no reader answers stand absent, so a caller cannot tell a
/// hybrid part or an Arm part from a production identity. The reader caches,
/// so the call runs its reader once per process at the first call and reads
/// nothing at a later one.
///
/// \pre none
/// \post The first four fields equal the library's own cpu reader's answer,
///       `coreType` is empty and `midr` is unset: no reader for either exists
///       in this feature (FR-003, spec D-13).
[[nodiscard]] SPEEDGUN_NG_EXPORT auto cpuIdentity() -> CpuIdentity;

/**
 * @brief One file of a table directory, as the source record states it
 * (specs/042-topdown FR-015, spec D-12, PC-12, data-model E-02).
 *
 * `name` is the name the embedded index gives the file, a bare file name, and
 * `manifestHash` is the digest the manifest line beside that name states, read
 * out of the vendored `RECORD` when the table was embedded. The report spells
 * the digest key `manifest_hash`.
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TableFile
{
  std::string name;
  std::string manifestHash;
};

/**
 * @brief Where the rows of a table came from (specs/042-topdown FR-015,
 * spec D-12, data-model E-03).
 *
 * `reference` is the reference the manifest's `ref:` line states, one value
 * for the whole tree the table is part of. `directory` is the selected
 * directory, spelled the way the mapping file spells a directory:
 * `arch/x86/skylakex/`. `files` holds one entry per file that contributed a
 * row, in the sorted order the embedder emits, and a file that contributed no
 * row stands in no record.
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownSource
{
  std::string reference;
  std::string directory;
  std::vector<TableFile> files;
};

/**
 * @brief One metric row of a table directory, published or refused
 * (specs/042-topdown FR-010, FR-016, FR-017, data-model E-04).
 *
 * `name`, `group`, `unit` and `threshold` carry the table's own four texts
 * byte for byte: the row's `MetricName`, its group text, which holds the
 * level token and the parent token, its `ScaleUnit` text and its
 * `MetricThreshold` text. The formula grammar reads no threshold shape, so no
 * part of this feature evaluates that text and an empty one names a row that
 * carries none (spec D-9, spec D-11).
 *
 * `formula` is the spine the metric layer publishes for the row, so a caller
 * folds it the way it folds a hand-composed expression. A published row
 * carries that spine and an empty `reason`; a refused row carries the
 * `MetricRefusal` kind and the reason text the metric layer answered with, and
 * no spine (FR-016).
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownRow
{
  std::string name;
  std::string group;
  std::string unit;
  std::string threshold;
  Expression<Dim<0, 0>> formula;
  bool published = false;
  MetricRefusal refusal = MetricRefusal::MALFORMED;
  std::string reason;
};

/**
 * @brief The listing a top-down selection reads: one directory's rows beside
 * the record of where they came from (specs/042-topdown FR-001, FR-015,
 * FR-016, data-model E-05).
 *
 * `rows` holds every row of the selected directory, published and refused
 * alike, in the order the loader read the directory's files. The loader drops
 * nothing and answers a verdict for every metric row, so a refused row stays
 * in the listing beside the published ones (spec D-20).
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownTable
{
  TopdownSource source;
  std::vector<TopdownRow> rows;
};

/// @brief The table of one identity: the directory the vendored mapping file
/// gives it, beside the rows of that directory.
///
/// The mapping file is matched row by row and the first row that hits names the
/// directory, so the identity's vendor, family, model and stepping decide the
/// answer between them. A core type and an `MIDR` join no key, so neither moves
/// a selection: no mapping-file row reads either (spec D-13).
///
/// The call reads no counter, opens no event, reads no clock and calls no cpu
/// reader of its own, so the same identity answers the same table in the same
/// process (FR-002, FR-009).
///
/// \pre none
/// \post An empty `source.directory` names an identity the mapping file
///       matched no row for, and `rows` then stands empty. Every row of the
///       selected directory answers a verdict, published or refused, in the
///       sorted file order the embedder emits (FR-001, FR-016).
[[nodiscard]] SPEEDGUN_NG_EXPORT auto topdownTable(const CpuIdentity& identity)
    -> TopdownTable;

/**
 * @brief One node of a top-down tree: a row the host publishes
 * (specs/042-topdown FR-010 to FR-014, FR-017, data-model E-06).
 *
 * `name` is the vendor-neutral name: one of the four shared names at level 1,
 * and `topdown.<vendor>.<table name>` below it. `vendorName` is the row's own
 * `MetricName`, printed and recorded beside the neutral name. `parent` is the
 * neutral name of the parent the row's group text states, and it is empty at
 * level 1. `level` is the row's own level token.
 *
 * `formula` is the row's spine, so a caller can fold it, and `events` holds
 * each spine leaf's address, the address the planner groups its passes on.
 * `threshold` is the table's own text, and nothing evaluates it
 * (FR-017, spec D-9).
 *
 * The struct is data: it has no call to guard. The shape no member may break
 * is the tree's own: a node's level stands one below its parent's level, and
 * its parent names a published row that the vector of the tree holds ahead of
 * this node.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownNode
{
  std::string name;
  std::string vendorName;
  std::string parent;
  int level = 0;
  Expression<Dim<0, 0>> formula;
  std::vector<std::string> events;
  std::string threshold;
};

/**
 * @brief One row of a level the host refuses: the library's refused row and
 * the record's refused row (specs/042-topdown FR-016, FR-031, data-model
 * E-07).
 *
 * One name serves both carriers, the way `TopdownNode` serves the library's
 * node and the record's node. The JSON key stays `refused` and its item keys
 * stay `name`, `kind` and `reason`, and this header is the authority for the
 * C++ names.
 *
 * `name`, `parent` and `level` are the row's identity and depth, and `parent`
 * is empty where the table names none. `refusal` and `reason` are the metric
 * layer's kind and text, copied unchanged.
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownRefusal
{
  std::string name;
  std::string parent;
  int level = 0;
  MetricRefusal refusal = MetricRefusal::MALFORMED;
  std::string reason;
};

/**
 * @brief The tree one selection answered, with the rows it refused
 * (specs/042-topdown FR-009, FR-010, FR-015, FR-016, FR-020, data-model
 * E-08).
 *
 * `source` is the record of the table the nodes came from and `depth` is the
 * asked level clamped to the deepest level the table holds. `amd` chooses the
 * fifth level-1 value and the 100 percent statement, and it stays inside the
 * library: the report carries no vendor flag.
 *
 * `nodes` holds every node of every level at or above `depth`, in table order,
 * and `refused` holds every row of those levels the host refuses. The two are
 * disjoint and together they cover every level row the table holds at or above
 * `depth`.
 *
 * The struct is data: it has no call to guard. The shape no member may break
 * is that no node stands that a row does not state.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownTree
{
  TopdownSource source;
  int depth = 0;
  bool amd = false;
  std::vector<TopdownNode> nodes;
  std::vector<TopdownRefusal> refused;
};

/**
 * @brief What a caller asks a selection for: one identity, one level, and the
 * table a test plants (specs/042-topdown FR-002, FR-009, FR-018, data-model
 * E-09).
 *
 * `table` stands unset in production, where the selection reads the table of
 * `identity`. A test hands in the rows it wants the answer to come from, which
 * is the seam a synthetic directory stands in for (PC-9).
 *
 * The struct is data: it has no call to guard.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownSelection
{
  CpuIdentity identity;
  int level = 1;  ///< The asked level, 1 to 6.
  std::optional<TopdownTable> table;
};

/// Why a selection answered no tree at all: the closed set of reasons the
/// vendored data gives (specs/042-topdown FR-004 to FR-007).
enum class TopdownUnsupport : std::uint8_t
{
  UNKNOWN_VENDOR,  ///< The vendored tree holds no table of this vendor.
  NO_DIRECTORY,  ///< The mapping file names no table for this identity.
  HYBRID,  ///< The selected directory publishes two core scopes.
  ARM,  ///< An Arm part, and no Arm table is vendored.
  NO_TOPDOWN_ROWS,  ///< The selected directory holds no top-down row.
};

/**
 * @brief The refusal a selection answers when it can answer no tree
 * (specs/042-topdown FR-004 to FR-007, FR-040, spec D-14, data-model E-10 and
 * E-11).
 *
 * `vendor` is the vendor text the message names, and `identity` is the
 * identity in the message's own spelling, `family 6 model 85 stepping 4` for
 * an x86 part and the `MIDR` for an Arm part. `detail` is the directory the
 * selection read, or the Arm sentence.
 *
 * The struct is data: it has no call to guard. The message is the one line the
 * harness prints, and nothing else carries these fields to a caller.
 *
 * \pre none
 * \post none
 * \invariant none
 */
struct SPEEDGUN_NG_EXPORT TopdownUnsupported
{
  TopdownUnsupport kind = TopdownUnsupport::UNKNOWN_VENDOR;
  std::string vendor;
  std::string identity;  ///< `family 6 model 85 stepping 4`, or the `MIDR`.
  std::string detail;  ///< The directory read, or the Arm sentence.
};

/**
 * @brief The tree one selection asks for, built from the rows of one table
 * (specs/042-topdown FR-002, FR-008 to FR-016, FR-020; data-model E-08).
 *
 * The answer follows from `options.identity` and from one table: the planted
 * `options.table` where one stands, and the table of the identity where none
 * stands. The call reads no counter, opens no event, reads no clock and calls
 * no cpu reader of its own, so two calls over one identity and one table answer
 * one tree (FR-002, FR-009). The rows reach the call as the verdicts the metric
 * layer answers, and a directory resolves once, so the pass that builds a tree
 * reads those verdicts as data.
 *
 * A row reaches a node when its group text carries a level token of `TopdownL1`
 * to `TopdownL6`, `PipelineL1` or `PipelineL2`, when that level stands at or
 * above the depth the answer states, and when its parent already stands in the
 * tree (FR-013, FR-014). A level-1 row roots a node and names no parent. A
 * deeper row names its parent with the `_group` token of its own group text:
 * Intel spells that token `tma_<name>_group` and AMD spells it `<name>_group`,
 * and the stem of either names another row of the table. A row whose token
 * stems to no row reaches no node, and no part of this feature invents a parent
 * for it (FR-013). The rows are walked level by level, so every parent names an
 * earlier element of `nodes`: a caller folds a parent before it folds the
 * children of that parent. A node's level stands one below its parent's level.
 * Measured over the vendored tables: 98 group tokens of `arch/x86/skylakex/`
 * and 111 of `arch/x86/sapphirerapids/` stem to a row of the same table, and
 * every one of them names a row exactly one level above.
 *
 * `depth` is the asked level clamped to the deepest level token the table
 * holds, so a table of level-1 rows answers level 1 whatever the ask (FR-008,
 * FR-020). A row the host refuses stands in `refused` with the metric layer's
 * kind and reason beside its own name, its parent and its level, and it builds
 * no node (FR-016, FR-031). A node carries the spine of its row and the address
 * of every leaf that spine reads, which is what a run groups its passes on
 * (FR-010, FR-021).
 *
 * An identity no table serves answers one `TopdownUnsupported` that carries a
 * kind and the fields that identity states: an `MIDR` names an Arm part, a
 * vendor text of neither vendor names an unknown vendor, and a table with no
 * directory names a mapping-file miss. `message()` owns the one line a caller
 * prints, and the two refusals a table's own rows state are T079 (FR-004 to
 * FR-007, FR-040, spec D-14).
 *
 * \pre `options.level` stands in 1 to 6, and `SG_REQUIRE` enforces that.
 * \post The answer is pure over the identity and the rows: no counter read, no
 *       event open, no clock read and no CPUID read stand behind it (FR-002,
 *       FR-009). `depth` is the asked level clamped to the deepest level the
 *       table holds (FR-008, FR-020), and no node stands that a row of the
 *       table does not state (FR-013).
 * \invariant none
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto topdownTree(
    const TopdownSelection& options)
    -> std::expected<TopdownTree, TopdownUnsupported>;

}  // namespace sg::counters

#endif  // SG_COUNTERS_TOPDOWN_HPP
