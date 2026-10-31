#ifndef SG_HARNESS_DETAIL_HOST_RECORD_CHECK_HPP
#define SG_HARNESS_DETAIL_HOST_RECORD_CHECK_HPP

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "host_record.hpp"

/**
 * @file host_record_check.hpp
 * @brief The record reader of specs/031: what one record reads, what one
 * tree answers about the catalog directory, and the check over the
 * committed records the thin reader program runs.
 *
 * Nothing here is public API (FR-040): the `validate` target and the two
 * thin programs under `tools/hostcheck/` are the surface, and the lines
 * and the exit status are the whole answer (FR-024, FR-037).
 *
 * \invariant no entry point in this header reads a clock, a cpu, `/proc`
 *            or `/sys`, so every rule holds on planted records and a
 *            planted tree on any runner (FR-026, FR-033, FR-035).
 */

namespace sg::detail
{

/// @brief specs/031 data-model.md E-26: the one word a record reads, the
/// verdict column of the line the reader prints (FR-024, FR-030).
enum class RecordVerdict
{
  VALID,
  BROKEN,
  STALE,
};

/// @brief The record schema, relative to whatever tree root the run
/// names, so one root answers the records, the mapfile and the guard
/// (FR-020, FR-028).
inline constexpr std::string_view kHostRecordSchemaRelPath =
    "docs/schema/host-record-v1.json";

/// @brief The mapfile of a tree, under the root the run names: the rows the
/// writer records a directory beside and the reader asks about again. The
/// library opens no mapfile of its own, and this is the path a run names when
/// that mapfile stands unreadable (FR-008, FR-020, FR-025).
inline constexpr std::string_view kMapfileRelPath =
    "external/pmu-events/arch/x86/mapfile.csv";

/**
 * @brief specs/031 data-model.md E-26: what the reader found for one
 * record: one reading, one line (FR-024, FR-030).
 *
 * The reader prints the three verdicts as the words FR-030 names:
 * `valid`, `broken` and `stale`.
 *
 * \invariant `cpuType` names a CPU type, read from `cpu.key`, or from the
 *            entry's own name with the `.json` suffix removed where the
 *            document yields no key (FR-024)
 * \invariant a VALID reading carries an empty `detail`, and the other two
 *            name the rule that fired beside the path, key or version it
 *            fired on (FR-024, FR-025)
 * \invariant a BROKEN reading names no staleness, because a record that
 *            breaks reports the break alone (FR-030)
 */
struct RecordReading
{
  std::string cpuType;
  RecordVerdict verdict {};
  std::string detail;
};

/// @brief specs/031 data-model.md E-27: the one answer the mapfile under
/// the run's tree root gives about one identity (FR-008, FR-025).
enum class DirectoryAnswer
{
  FOUND,
  NO_ROW,
  UNREADABLE_MAPFILE,
};

/**
 * @brief specs/031 data-model.md E-27: the answer to one directory
 * question, one mapfile open behind a run of questions (FR-008, FR-025,
 * plan R-06).
 *
 * A FOUND answer carries the directory the mapfile selects and the reader
 * compares it against nothing, because the record states the directory
 * that was true when the writer wrote it (FR-008, FR-025).
 *
 * \invariant `directory` carries the selected path under FOUND and stands
 *            empty under the two answers that name none (FR-008)
 */
struct DirectoryChoice
{
  DirectoryAnswer answer;
  std::string directory;
};

/// @brief The question a record asks about its own identity, answered by
/// the chooser a factory builds or by the function a test plants
/// (FR-025, AC-1).
using DirectoryChooser = std::function<DirectoryChoice(const HostIdent&)>;

/// @brief The builder of that chooser for one tree root: the reader entry
/// point asks it for the chooser of the root its parse read, so the mapfile
/// open that answers rule 4 stands in the program, the one party allowed to
/// reach the matcher (E-27, FR-008, FR-025, plan R-04, R-06).
using DirectoryChooserFactory =
    std::function<DirectoryChooser(const std::filesystem::path&)>;

/**
 * @brief The record one file holds: simdjson reads the bytes and the
 * schema at `schemaPath` guards them, the only reader of JSON this
 * feature has (E-24, FR-020 to FR-022, FR-030).
 *
 * \post a document the guard accepts answers its record, with the fields
 *       the document spells (FR-020, FR-021)
 * \post a document that fails the guard in key set, key order, kind or
 *       size answers no record, the reading of that entry carrying the
 *       BROKEN verdict with the `host-record` label beside what the
 *       guard reported (FR-021, FR-030)
 */
[[nodiscard]] auto readHostRecord(const std::filesystem::path& file,
                                  const std::filesystem::path& schemaPath)
    -> std::optional<HostRecord>;

/**
 * @brief Every staleness cause one record carries, asked in the order
 * the reader names them (E-26, FR-025).
 *
 * The four rules: `recordVersion` behind the version the pinned schema
 * names, an evidence `path` absent under the tree, a cited `line` absent
 * in that file, and a chooser answer of NO_ROW (FR-025).
 *
 * \pre `record` passed the schema, so its fields carry values
 * \pre `treeRoot` names the tree the evidence rows stand in
 * \post every cause the record carries stands named, in rule order, each
 *       reason naming the two versions, the path, or the key and the
 *       identity it names (FR-025)
 * \post an answer that stands empty names no cause, the reading the
 *       reader prints as `valid` (FR-025, FR-027)
 * \invariant it asks `chooser` one question per record, and an
 *           UNREADABLE_MAPFILE answer reaches no reason here, because the
 *           run answers it alone (FR-004, FR-025)
 */
[[nodiscard]] auto staleReasons(const HostRecord& record,
                                const std::filesystem::path& treeRoot,
                                const DirectoryChooser& chooser)
    -> std::vector<std::string>;

/**
 * @brief specs/031 data-model.md E-26, plan D-3: the reader's answer for
 * one record root: the readings of the records, or the one run-level path
 * that ended the run.
 */
struct RecordCheck
{
  // One reading per `.json` entry directly under the record root, none on
  // a refusal (FR-024, FR-029).
  std::vector<RecordReading> readings;
  // The run-level path, set at most once: the mapfile that stood
  // unreadable or the entry that could not be opened, the case that
  // leaves the readings empty (FR-004, FR-025, FR-029).
  std::optional<std::string> refusal;
};

/**
 * @brief The reading of every record under one root: the listing, the
 * schema guard, the staleness rules, and the readings that become the
 * lines, printed by nobody here (FR-024 to FR-030, plan D-3).
 *
 * \pre `treeRoot` and `recordRoot` stand absolute, and `schemaPath`
 *      arrives resolved from the same root as
 *      `treeRoot / kHostRecordSchemaRelPath` so the two paths cannot
 *      disagree, with a readable schema at that path standing on the
 *      caller (FR-020)
 * \pre nothing asks the record root to hold an entry (D-10)
 * \post every returned reading names a CPU type, and a VALID reading
 *       carries an empty `detail` (FR-024)
 * \post an UNREADABLE_MAPFILE answer sets `refusal` to the mapfile path
 *       and leaves `readings` empty, so the empty set and the unanswered
 *       rule stay two different answers (FR-004, FR-025)
 * \post a `.json` entry that cannot be opened sets `refusal` to that
 *       entry (FR-029)
 * \invariant this entry point owns the listing: the `.json` entries
 *           directly under `recordRoot` and no other entry there, read
 *           through the error-code form of the directory iterator, so a
 *           root that stands absent and a root that holds no `.json`
 *           entry answer the same empty pair (D-10, FR-029)
 * \invariant it opens no mapfile of its own and asks `chooser` only per
 *           record, so one open stands behind a run that has records and
 *           no open stands behind a run that has none (plan R-06)
 * \invariant a schema check that fails for any reason, a read error
 *           included, answers through the one mapping that turns a failed
 *           check into the BROKEN reading (FR-004, FR-020)
 */
[[nodiscard]] auto checkRecords(const std::filesystem::path& recordRoot,
                                const std::filesystem::path& treeRoot,
                                const std::filesystem::path& schemaPath,
                                const DirectoryChooser& chooser) -> RecordCheck;

/**
 * @brief The reader's own run: the parse of `--root` and `--records`, the
 * schema under that tree root, the check over the record root, and the
 * lines the answer becomes (FR-004, FR-024 to FR-030, FR-037).
 *
 * The thin program under `tools/hostcheck/` hands over its `argc`, its
 * `argv` and the factory that builds the chooser of the root its parse
 * reads, and it maps the answered status to the process exit, the way the
 * standing refusal shape leaves the exit to `main` (FR-028, R-09).
 *
 * \pre `argc` stands at zero or above and `argv` carries that many words
 *      plus the terminating null, the shape the process hands `main`
 * \pre `factory` answers a chooser for the tree root it names, the program
 *      handing over the builder that opens
 *      `<treeRoot>/external/pmu-events/arch/x86/mapfile.csv` at that
 *      chooser's first question, because the harness library reads no
 *      counters internal header (plan R-04, R-06)
 * \post status 0: one line per reading naming the record, the CPU type
 *       and `valid`, every record read standing valid (FR-024, FR-027)
 * \post status 2: one line naming the case that answered it, an unusable
 *       argument on standard error, the schema absent under the tree, the
 *       mapfile unreadable, a record root that holds no `.json` entry, an
 *       entry that cannot be opened, or a reading that reads `broken` or
 *       `stale` (FR-004, FR-020, FR-025, FR-027, FR-029)
 * \invariant it answers in the order the contract states: the parse, then
 *           the schema at `treeRoot / kHostRecordSchemaRelPath`, then
 *           `checkRecords`, so a run-level refusal stands alone with no
 *           per-record line beside it (FR-004, FR-020, FR-025)
 * \invariant no option names a schema path: the derivation from the tree
 *           root answers it, so one root carries the records, the mapfile
 *           and the guard (FR-004, FR-020)
 * \invariant it prints one line per reading and the status follows the
 *           lines alone, so every record it read stands reported
 *           (FR-027, FR-030)
 * \invariant it asks `factory` once for the root its parse read, so one
 *           mapfile open stands behind the run and none behind a run with
 *           nothing to read (plan R-06)
 */
auto hostRecordCheckMain(int argc,
                         char** argv,
                         const DirectoryChooserFactory& factory) -> int;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_HOST_RECORD_CHECK_HPP
