#ifndef SG_HARNESS_DETAIL_HOST_RECORD_HPP
#define SG_HARNESS_DETAIL_HOST_RECORD_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal.hpp"

/**
 * @file host_record.hpp
 * @brief The host record of specs/031: the identity, machine and
 * evidence records one host writes about itself, the machine block of a
 * standing layout, and the write path behind the thin writer program.
 *
 * Nothing here is public API (FR-040): the `validate` target and the two
 * thin programs under `tools/hostcheck/` are the surface (FR-001,
 * FR-037), and `docs/schema/host-record-v1.json` guards the bytes the
 * write path produces (FR-019, FR-020).
 *
 * \invariant no declaration here reads a host: the identity, the layout
 *            and the catalog directory arrive as values the program read,
 *            so every rule here holds on planted values on a runner that
 *            reads no CPUID (R-04, FR-032, FR-035).
 * \invariant no declaration here reads a clock, and no bytes it writes
 *            carry a wall-clock instant (FR-033).
 */

namespace sg::detail
{

/// @brief specs/031 FR-019: the record format this writer emits and the
/// reader's first staleness rule compares against (FR-019, FR-025). One
/// constant carries it for both sides, so a bump moves the written bytes,
/// the rule and the schema row in one change, and cannot leave a committed
/// record set stale because a second copy of the text stood elsewhere.
inline constexpr std::string_view kHostRecordVersion = "1.0";

/// @brief specs/031 FR-018: the size cap of one record document, in bytes.
/// One constant carries it for both sides, so the writer that refuses a
/// document above it and the reader that breaks one it is handed hold the
/// same figure, and a run cannot read back a record its own writer would
/// not have left standing (FR-018, FR-030, R-09).
inline constexpr std::size_t kHostRecordCapBytes =
    static_cast<std::size_t>(16U) * 1024U;

/**
 * @brief specs/031 data-model.md E-21: the CPU identity as the detection
 * renders it and as the record names it (FR-007 to FR-010).
 *
 * The program fills every field: `pmuIdentCurrent()` answers the four
 * identity figures, `mapfileKey` renders `key`, and
 * `pmuSelectDirectory` over the mapfile under the root the run names
 * answers `directory` (FR-007, FR-008).
 *
 * \invariant `key` equals `<vendor>-<family>-<MODELhex>[-<steppinghex>]`,
 *            the record key `mapfileKey` renders, and the record file
 *            name adds the `.json` suffix (FR-009)
 * \invariant `stepping` stands empty exactly where the host answers no
 *            leaf 1, and the record writes `null` there (FR-007, FR-013)
 * \invariant an empty `vendor`, a negative `family` or `model`, or an
 *            empty `directory` names no CPU, the identity FR-010 refuses
 *            (FR-008, FR-010)
 */
struct HostIdent
{
  std::string vendor;
  int family = -1;
  int model = -1;
  // specs/031 FR-007, FR-013: absent where the host answers no leaf 1,
  // one of the two nullable keys of the document beside `nodes`.
  std::optional<int> stepping;
  // The record key `mapfileKey` renders: the CPU type the reader names,
  // the name the record file carries, present for an unnamed identity too
  // because the refusal names it (FR-007, FR-009, FR-010).
  std::string key;
  // The catalog directory the mapfile selects for this identity, empty
  // where the mapfile names nothing (FR-008).
  std::string directory;
};

/**
 * @brief specs/031 data-model.md E-22: the machine shape of the run, the
 * same facts the `--topology` listing prints (FR-012 to FR-014).
 *
 * `hostMachineFromLayout` answers it, so each count, the mask text and
 * the two row lists carry the listing's own readings (FR-014).
 *
 * \invariant an answered block carries at least one package, L3 group,
 *            core and thread, the shape of a layout that stands
 *            readable (FR-012, FR-034)
 * \invariant `nodes` stands absent exactly where the machine memory
 *            reads unavailable, the JSON `null` behind the standing
 *            `node: not available` line (FR-013)
 * \invariant no row names a cpu the process mask leaves out, the listing
 *            rule that skips a masked node (FR-013, FR-034)
 */
struct HostMachine
{
  /// @brief specs/031 data-model.md E-22: the row of one L3 group, its
  /// ordinal and the cpu range text inside the mask (FR-012).
  struct L3Row
  {
    std::size_t ordinal = 0;
    std::string cpus;
  };

  /// @brief specs/031 data-model.md E-22: the row of one memory node,
  /// the number the read states and the cpu range text the mask leaves
  /// (FR-013).
  struct NodeRow
  {
    std::uint32_t number = 0;
    std::string cpus;
  };

  std::uint32_t packages = 0;
  std::size_t l3Groups = 0;
  std::size_t cores = 0;
  std::size_t threads = 0;
  // The affinity mask in the `appendRangeText` spelling, the text of the
  // standing `mask=` field (FR-012; source/harness/topology.cpp:529).
  std::string mask;
  std::vector<L3Row> l3;
  std::optional<std::vector<NodeRow>> nodes;
};

/**
 * @brief specs/031 data-model.md E-23: one recorded run the record
 * points at, authored by hand and kept by a refresh (FR-015, R-08).
 *
 * The reader looks for `line` inside the file `path` names under the
 * tree it checks, the anchor of staleness rules 2 and 3 (FR-025).
 *
 * \invariant `command` and `line` stand non-empty, and the row cites the
 *            file with no figure that file already holds (FR-018)
 * \invariant `path` stands repository-relative and inside the tree the
 *            record lives in (FR-015, FR-025)
 */
struct EvidenceRow
{
  std::string path;
  std::string command;
  int exit = 0;
  std::string line;
};

/**
 * @brief specs/031 data-model.md E-24: the document one host writes to
 * `<out>/<key>.json`, guarded by the pinned schema (FR-016, FR-019 to
 * FR-023).
 *
 * A refresh reads the standing document first, replaces `cpu` and
 * `machine`, and keeps `evidence` row for row in the stored order
 * (FR-006, R-08).
 *
 * \invariant the four root keys stand in the order `recordVersion`,
 *            `cpu`, `machine`, `evidence`, the order the schema spells
 *            in `x-order` and the renderer's call order (FR-020, FR-021)
 * \invariant the text stays within 16 KiB and carries no wall-clock
 *            instant, no run identifier and no authored host or person
 *            name (FR-018, FR-033, FR-042)
 */
struct HostRecord
{
  // The format version this writer speaks, `1.0` at first (FR-019).
  std::string recordVersion;
  HostIdent cpu;
  HostMachine machine;
  // The recorded runs, an empty list allowed: the lane commits its own
  // record with none (FR-041, SC-011).
  std::vector<EvidenceRow> evidence;
};

/**
 * @brief The machine block of one layout, the counts, mask text, L3 rows
 * and node rows the standing listing prints (E-22, FR-012 to FR-014,
 * R-05).
 *
 * The block reads no host of its own and touches no clock: it reads the
 * layout it is handed, so the listing and the record agree by
 * construction (FR-014, FR-033).
 *
 * \pre `machine` stands readable: a failed layout read answers its own
 *      refusal before the call, the way the state model of data-model.md
 *      states it (FR-011)
 * \post the four counts equal the counts of `machine`, and `mask`
 *       carries the `appendRangeText` spelling of its mask (FR-012,
 *       FR-014)
 * \post `l3` carries one row per L3 group of `machine`, in the order of
 *       that vector (FR-012)
 * \post `nodes` stands absent exactly where `memoryReadable` stands
 *       false, and otherwise carries one row per node whose cpu set the
 *       mask leaves, in the order of that vector (FR-013)
 */
[[nodiscard]] auto hostMachineFromLayout(const MachineLayout& machine)
    -> HostMachine;

/**
 * @brief specs/031 data-model.md, plan R-04: the host facts one write
 * reads, the identity of E-21 and the layout whose block E-22 states.
 *
 * The thin writer program reads all three from the host and hands them over
 * the seam, so the library holds no CPUID read, no topology walk and no
 * mapfile open (FR-007, FR-012, R-04).
 */
struct HostFacts
{
  HostIdent ident;
  MachineLayout layout;
  /// @brief Whether the mapfile of the tree answered rows the matcher could
  /// weigh. It stands false where the file stood absent, stood unreadable, or
  /// held a row whose pattern the matcher cannot compile, and true where the
  /// rows were weighed, even where no row named a directory (FR-004, FR-008,
  /// FR-010). The seam owns the distinction, being the one party that opens the
  /// mapfile, and it carries the difference between a file the run cannot read
  /// and an identity that row set names no CPU for.
  bool mapfileReadable = true;
};

/// @brief The one seam `hostRecordWriteMain` asks for its host facts. It
/// answers for the tree root the parse named, because that root holds
/// the mapfile whose selected directory the identity reports (FR-008,
/// R-06). Where that mapfile answered no rows at all, the answer says so
/// beside the facts it carries (FR-004). A test plants its own answer, the
/// way the standing seams let a test install a layout (FR-032, FR-035).
using HostFactsSeam = std::function<HostFacts(const std::filesystem::path&)>;

/**
 * @brief The whole write path of specs/031: the parse, the host facts,
 * the refresh, the render and the one file (FR-002 to FR-011, FR-016,
 * FR-023, FR-037, R-08, R-09).
 *
 * The thin program under `tools/hostcheck/` hands over its `argc`, its
 * `argv` and the seam, and maps the answered status to the process exit
 * (FR-037). The entry point prints the one line of its own and leaves
 * the exit to that `main` (R-09).
 *
 * \pre `argc` stands at zero or above and `argv` carries that many words
 *      plus the terminating null, the shape the process hands `main`
 * \pre `facts` answers the identity and the layout of the root it
 *      receives, and the call asks it only once `--root` and `--out`
 *      answer their values (FR-002, FR-037)
 * \post status 0: one line names the CPU type and the path written, and
 *       that path holds the document under `cpu.key` with the `.json`
 *       suffix (FR-005, FR-009)
 * \post status 2: one line stands on standard error naming the identity,
 *       the failed read or the argument at fault, and no partial record
 *       stands behind it (FR-004, FR-010, FR-011)
 * \invariant the parse runs before the seam stands asked, so an argument
 *            no arm accepts reads no host (FR-004, FR-037)
 * \invariant a refresh keeps the standing `evidence` rows in their
 *            stored order and replaces `cpu` and `machine` (FR-006, R-08)
 * \invariant the document renders in memory and passes the size cap
 *            before the one write, so a record over the cap leaves the
 *            standing bytes alone (FR-011, FR-018, R-09)
 */
auto hostRecordWriteMain(int argc,
                         char** argv,
                         const HostFactsSeam& facts) -> int;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_HOST_RECORD_HPP
