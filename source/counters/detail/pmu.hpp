#pragma once

// Private seam of the Linux PMU provider (specs/007-counters-and-timers,
// US6, T045..T048). Never installed. The seam separates table parsing
// (simdjson-private TU: table_parse.cpp), encoding (encode.cpp), and
// group I/O (group_io.cpp) from the provider assembly
// (provider.cpp). FR-013/004 privacy: the sole <simdjson.h> include of
// source/counters/ lives in table_parse.cpp; nothing here leaks JSON
// types.

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <sys/types.h>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"

namespace sg::counters::detail
{

// One architecture event-table entry, semantic fields only: the
// encoder composes them with the running kernel's bit layouts
// (FR-037). Metric-definition rows are not entries: they ship as
// catalog data for a future release; they are no countables (US6).
struct PmuTableEntry
{
  std::string name;
  std::string description;
  // The row's Unit scope, empty where the table carries no Unit key. The
  // empty string is the core scope the placement rule reads, and a
  // placeholder spelling like "none" made every core row name a class no
  // device publishes, so a core row reached no device and the core
  // catalog carried only the kernel's own aliases (FR-014, FR-015, FR-019,
  // D-08).
  std::string unit;
  // Semantic config fields: format-field name to value. The
  // convention-mapped names: "event" (EventCode), "umask" (UMask),
  // plus any other JSON key that names a format field.
  std::vector<std::pair<std::string, std::uint64_t>> fields;
};

// CPU identification read once at open (FR-038): vendor string, family
// decimal, model hex, matching the upstream mapping file's spelling.
struct PmuIdent
{
  std::string vendor;
  int family = -1;
  int model = -1;
};

// The host's CPU identification (table_parse.cpp; CPUID-based, cached).
[[nodiscard]] auto pmuIdentCurrent() -> PmuIdent;

// The architecture directory the mapping file selects for `id`: the
// first row whose regex matches, parsed once lazily (FR-038). Empty
// when no row matches; parse failures surface as an empty table.
[[nodiscard]] auto pmuSelectDirectory(const PmuIdent& id) -> std::string;

// The same selection over an already-open mapping file, so a fixture
// supplies the rows (T066). The header line is skipped, a row without
// the three leading columns is skipped, and a row whose pattern does
// not compile throws `std::regex_error`: the pinned mapfile is
// regex-validated when it is re-pinned
// (`tools/pmu_events/update_pmu_events.py`), so a row that fails to
// compile is corrupt data and a corrupt table is worse than a loud
// failure.
[[nodiscard]] auto pmuSelectDirectory(std::istream& mapfile,
                                      const PmuIdent& id) -> std::string;

// The parsed event table for `directory`, lazily and once per
// directory (FR-038). Entries carry semantic fields only.
[[nodiscard]] auto pmuLoadTable(const std::string& directory)
    -> const std::vector<PmuTableEntry>&;

// One event-table file parsed into `out` (table_parse.cpp). The seam
// entry for a synthetic fixture: the pinned tree at
// `external/pmu-events/` carries hex-string values and array-shaped
// files, so a fixture is the only way CI reaches the numeric and
// object-shaped parses (T066).
void pmuParseTableFile(std::string_view path, std::vector<PmuTableEntry>& out);

// The field/value pairs a kernel event_attr text carries (FR-037): the
// text is "field=value[,field=value...]" and every value is
// hexadecimal or decimal. A piece with no `=`, an empty value, a
// non-hexadecimal character, and a value decoding to zero carry no
// field, so the accepting pairs are the whole result.
[[nodiscard]] auto parseAttr(const std::string& text)
    -> std::vector<std::pair<std::string, std::uint64_t>>;

// The event code as the catalog spells it: hexadecimal with a `0x`
// prefix and no leading zero nibble, so zero reads as "0x0".
[[nodiscard]] auto toHex(std::uint64_t value) -> std::string;

// The catalog description of one kernel event alias: the event_attr
// text the kernel publishes, verbatim (FR-037). Empty when the kernel
// publishes no text for the alias.
[[nodiscard]] auto aliasDescription(const std::string& name,
                                    const std::string& text) -> std::string;

// The catalog description of one vendored table row: the table's own
// prose, its event code when the row carries no prose, and the Unit
// scope label when the table names one (FR-017, FR-038).
[[nodiscard]] auto tableDescription(const PmuTableEntry& entry) -> std::string;

// A mapping-file pattern with every POSIX character class translated
// to its ECMAScript spelling (FR-038). std::regex is ECMAScript, and
// libstdc++ happens to accept `[[:name:]]` there; the other standard
// libraries do not.
[[nodiscard]] auto toEcma(std::string_view pattern) -> std::string;

// One bit range of a sysfs format field: which config word, inclusive
// low and high bit.
struct FormatRange
{
  int configWord = 0;
  int low = 0;
  int high = 0;
};

// The bit layout of one named format field (encode.cpp): parses
// "config:0-7,32-35" and friends. False when the file does not
// describe a config-bit field.
[[nodiscard]] auto parseFormatField(std::string_view spec,
                                    std::vector<FormatRange>& out) -> bool;

// Compose one entry's config words from semantic fields and the
// kernel's format layouts: every referenced field must exist, else
// nothing is half-composed and the entry is `not_encodable`
// (FR-037). On success `words` holds the per-config-word values.
//
// A field naming a register filter resolves like any other field: the
// parser records the register value under the format its index names, and
// the bit range that value lands in belongs to the device this function is
// handed. The table the row came from names no range. A device publishing
// no format of that name refuses the row whole, so the row publishes
// `not_encodable` and the count never appears without its filter (FR-010,
// FR-012, FR-013, D-05, D-06).
[[nodiscard]] auto pmuComposeConfig(
    const std::vector<std::pair<std::string, std::uint64_t>>& fields,
    const std::vector<std::pair<std::string, std::vector<FormatRange>>>&
        formats,
    std::vector<std::pair<int, std::uint64_t>>& words) -> bool;

// Probe one config through a test-open against the PMU `type` for one
// target kind (provider.cpp, FR-039): the availability the kernel grants
// this caller for this event. The probe answers `countable`,
// `not_encodable`, or `permission_blocked`, which is the closed set of
// states a pmu catalog entry carries. `Availability::ABSENT` belongs to
// a provider that declares a leaf absent from the object it seeds, and
// a device the kernel does not publish is dropped at discovery, so no
// entry carries it. `where` names the target kind, so a probe runs once
// per kind the entry's mask admits (FR-022).
[[nodiscard]] auto pmuProbe(
    int type,
    const std::vector<std::pair<int, std::uint64_t>>& words,
    const Target& where) -> Availability;

// One catalog entry the provider built: the composed config words, the
// description, the probed availability, and the target kinds that probe
// settled (FR-037, FR-039, FR-021).
struct PmuEntry
{
  std::string name;
  std::string description;
  // Composed per-config-word values. Empty for the enabled and running
  // leaves and for an entry the encoder refused, so `is_time_pair`
  // tells the two apart.
  std::vector<std::pair<int, std::uint64_t>> words;
  bool isTimePair = false;
  Availability avail = Availability::NOT_ENCODABLE;
  ReadMode mode = ReadMode::SYSCALL;
  // The target kinds the availability probe settled this entry on, and no
  // others, so the catalog names a kind only where that kind's own probe
  // counted the entry (FR-021). Zero for the enabled and running leaves,
  // which no probe runs for, and for every entry the probe refused,
  // which names no kind in any case.
  TargetMask probedKinds = 0;
};

// Records the target kinds the availability probe settled the seeded leaf
// at `address` on, keyed by that canonical address. FR-021 states the
// seeding surface (`CatalogSeed`) gains no field and the availability state
// stays one enumeration, so the per-kind verdicts ride beside the tree.
// The provider records them where it enumerates and the catalog reads them
// where it fills `CatalogEntry::targets`. Only a
// leaf the probe settled some kind on is recorded, and an address this
// table does not hold names no kind the probe settled (FR-021).
void noteProbedKinds(const std::string& address, TargetMask probed);

// The kinds the probe settled the leaf at `address` on, and zero where it
// settled none there. The lookup allocates nothing, so the catalog reads a
// leaf's kinds inside the `noexcept` mask decision (FR-021).
[[nodiscard]] auto probedKindsAt(const std::string& address) noexcept
    -> TargetMask;

// One event-source device: its canonical path, its PMU type, the bit
// layouts it publishes, and the merged catalog in seed order (kernel
// aliases first, then the vendored table; a kernel alias wins a name
// conflict, FR-037).
struct PmuDevice
{
  std::string path;  // canonical spelling, the sysfs device name
  std::string description;
  int type = -1;
  std::vector<std::pair<std::string, std::vector<FormatRange>>> formats;
  std::vector<PmuEntry> entries;
  // True when the device discloses the enabled/running time pair as
  // ordinary leaves, which the fold layer reads for the multiplex
  // ratio (FR-019, FR-041, C-PRO-6).
  bool hasTimePair = false;
  // True when the device binds one processor for every task, which is what
  // an uncore device does. A device-scoped entry refuses a per-task target
  // by its own scope, which is a different refusal from an encoding one,
  // so the provider answers `SCOPE_REFUSED` for that kind and runs no
  // syscall (FR-021, FR-022).
  bool deviceScoped = false;
};

// The released provider state (T047). Off Linux every device list is
// empty and the catalog is the whole reduced difference (FR-042).
struct PmuState
{
  std::vector<PmuDevice> devices;
};

// Opens the syscall-mode window for `leaves` (group_io.cpp, FR-041):
// one group per device, one read per group leader per sampling action.
// Null when a leaf names no device or no countable entry.
[[nodiscard]] auto pmuOpenWindow(const PmuState& state,
                                 const LeafSet& leaves,
                                 const Target& where)
    -> std::unique_ptr<WindowReader>;

// The outcome of one mapped-page read attempt (FR-040). A read from a
// thread other than the one that opened the context falls outside these
// verdicts: the two context reads below compare the calling thread
// against `FastContext::owner` and report a contract violation
// (FR-031, FR-040).
enum class FastReadVerdict : std::uint8_t
{
  OK,
  NOT_ALLOWED,  // no capability bit, or no valid counter index
  UNSTABLE  // the page sequence moved; the caller retries
};

// The width the kernel publishes counters at for a page that publishes
// none. It is the fallback, never the mask: a page that publishes
// `pmc_width` is read at that width, so a host whose counters differ is
// measured correctly (FR-040, R-011).
constexpr std::uint32_t kRnpmcCounterWidth = 48;

// The highest counter index the rdpmc instruction is given. The kernel
// publishes a one-based index in the event page and the caller reads
// `rdpmc(index - 1)`, so an index past this names no counter any host
// has. The gate bounds the instruction operand and states no property of
// any page (FR-040, R-011).
constexpr std::uint32_t kRnpmcMaxIndex = 1024;

// The pure halves of the mapped-page protocol over the fields the
// kernel's event page publishes (FR-040, R-011, T066; plan.md Coverage
// strategy names these the functions a synthetic page fixture covers in
// CI):
//   - the one-based index gate, over `index` alone;
//   - the counter width, defaulting to kRnpmcCounterWidth when the page
//     publishes none;
//   - the seqlock comparison, over the sequence before and after.
[[nodiscard]] auto fastIndexValid(std::uint32_t index) noexcept -> bool;

[[nodiscard]] auto fastCounterWidth(std::uint16_t published) noexcept
    -> std::uint32_t;

[[nodiscard]] auto fastPairStable(std::uint32_t sequenceBefore,
                                  std::uint32_t sequenceAfter) noexcept -> bool;

// The decode half of the protocol, over values the caller already
// sampled: the page sequence before and after, the one-based index, the
// capability, the raw instruction result, the kernel offset, and the
// counter width. The caller reads the instruction only for a nonzero
// index, so the gate here costs a page load and never an instruction
// (FR-040, R-011).
[[nodiscard]] auto fastDecode(std::uint32_t sequenceBefore,
                              std::uint32_t sequenceAfter,
                              std::uint32_t index,
                              std::uint64_t capability,
                              std::uint64_t raw,
                              std::int64_t offset,
                              std::uint32_t width,
                              std::uint64_t& value) -> FastReadVerdict;

// The probe's verdict over the three fields the mapped event page
// publishes: the capability, the index, and the width (FR-023, R-011).
// Writes the refusal the catalog discloses and answers whether the
// mapped-page read is usable on this host.
[[nodiscard]] auto fastProbeAllows(bool capabilityGranted,
                                   std::uint32_t index,
                                   std::string& refusal) -> bool;

// One per-thread fast-read context: the event file descriptor and the
// one page the kernel maps for it. `owner` names the thread that opened
// it, and the binding is checked at both read sites below, because the
// event counts one task and the page publishes no marker of the reading
// thread, so a foreign read hands back another task's count as a valid
// point (FR-031, FR-040). The plan checks the same binding, and that
// check is semantic-gated, so it is absent from a release build.
struct FastContext
{
  int fd = -1;
  void* map = nullptr;
  std::size_t mapLength = 0;
  std::thread::id owner {};
  // The processor a cpu-target plan bound this context to, or -1 for a
  // thread-bound plan. A cpu-pinned context counts one processor for
  // every task, so a sampling thread that migrated away reads another
  // processor's event or none at all, and the read carries the
  // pinning precondition (FR-045).
  int pinnedCpu = -1;

  /**
   * @brief Releases the mapping and the descriptor the context holds.
   *
   * Every exit path releases: a value that leaves scope, a window the
   * partial open refuses, and a window the plan destroys. Closing a
   * context that owns nothing touches nothing, so the release runs on any
   * host and needs no granted event (FR-013, FR-015).
   *
   * \pre none
   * \post The context owns no mapping and no descriptor.
   */
  ~FastContext();
};

// The pid and cpu a `perf_event_open` for `where` binds to: a
// thread-bound plan counts the calling thread on any cpu, and a
// cpu-pinned plan counts that cpu for every task (FR-024, FR-031). Both
// read modes bind the same way, so a plan reads what its target names
// whichever mechanism the catalog discloses.
[[nodiscard]] inline auto leaderPid(const Target& where) noexcept
    -> std::pair<pid_t, int>
{
  // LCOV_EXCL_BR_START : coverage exclusion (T140): both arcs. A cpu-pinned
  // plan binds an event with this arm, and a plan reaches one only where its
  // leaf is countable, which is a granted `perf_event_open`: a runner whose
  // `perf_event_open` is refused reports no countable leaf, so
  // `counters_pmu_test.cpp`'s cpu-target scenario skips and this arm runs
  // there; the host that grants the syscall runs it on every pinned plan.
  if (where.kind == TargetKind::CPU) {  // LCOV_EXCL_BR_LINE
    return {-1, where.cpu};  // LCOV_EXCL_LINE
  }
  return {0, -1};
  // LCOV_EXCL_BR_STOP
}

// The kernel's own self-monitor time recipe, over the fields the page
// publishes and the cycle counter the caller sampled. The computation is
// the kernel's: `linux/perf_event.h:669-688` documents the quotient, the
// remainder, and the delta, `:608-630` documents the sequence that gathers
// the fields, and `:717` documents the short-counter correction that sits
// on top. The header states the condition as `cap_usr_time && enabled !=
// running`, so an equal pair needs no delta and a page publishing no
// capability bit has no recipe to run (FR-007, FR-008, FR-009, FR-024).
struct EventTimeFields
{
  // The two capability bits the recipe is gated on. `cap_user_time` says
  // the shift, mult, and offset fields are used; `cap_user_time_short`
  // says the cycle and mask fields are used.
  bool cap_user_time = false;
  bool cap_user_time_short = false;
  std::uint64_t time_enabled = 0;
  std::uint64_t time_running = 0;
  // The page index, which is non-zero for every counter the instruction
  // reads. The kernel adds the delta to the running pair only where the
  // index is non-zero, because a page with no index has no running time
  // of its own to correct.
  std::uint32_t index = 0;
  std::uint16_t time_shift = 0;
  std::uint32_t time_mult = 0;
  std::uint64_t time_offset = 0;
  std::uint64_t time_cycles = 0;
  std::uint64_t time_mask = 0;
  // The cycle counter the caller sampled with the instruction. A library
  // read takes it from `rdtsc`; a synthetic page hands it over.
  std::uint64_t cyc = 0;
};

// The recipe's answer: the pair to publish, and whether a delta was
// computed at all. A caller that sees `applied` false publishes the raw
// pair, which is what the page states and the whole answer on a host whose
// kernel grants no time capability.
struct EventTimePair
{
  std::uint64_t enabled = 0;
  std::uint64_t running = 0;
  bool applied = false;
};

/**
 * @brief Applies the kernel's time recipe to a page's own fields.
 *
 * The computation is the one `linux/perf_event.h` documents at lines
 * 669-688: the quotient is the cycle counter shifted down by the page's
 * shift, the remainder is what the shift keeps, and the delta is the
 * offset plus the quotient times the multiplier plus the scaled
 * remainder. The delta is added to the enabled pair always, and to the
 * running pair only where the page index is non-zero, exactly as the
 * header states.
 *
 * Where the page publishes the short-counter capability, the cycle
 * counter is first narrowed by the header's own correction at line 717,
 * `cyc = time_cycles + ((cyc - time_cycles) & time_mask)`, because the
 * hardware clock is narrower than the counter there. That correction is
 * applied before the recipe and not after it, which is what the header
 * means by an explicit correction on top of the full form.
 *
 * A page publishing no time capability, or one whose enabled and running
 * counts already agree, has no delta to compute. The pair is copied
 * exactly as the page published it and `applied` reports false, so a
 * caller can tell a corrected pair from a raw one.
 *
 * Pure over the fields it is handed, so a synthetic page reaches it in CI
 * with no mapping and no instruction (T005, T014, T015, FR-007, FR-008,
 * FR-009).
 *
 * @param fields The page's own fields and the sampled cycle counter.
 * @return The pair to publish, with `applied` naming whether a delta was
 *         computed.
 */
[[nodiscard]] inline auto fastTimePair(const EventTimeFields& fields) noexcept
    -> EventTimePair
{
  EventTimePair pair {};
  pair.enabled = fields.time_enabled;
  pair.running = fields.time_running;
  // The header's own condition. An equal pair means the event was never
  // multiplexed, so the page's counts already measure the whole span.
  if (!fields.cap_user_time || fields.time_enabled == fields.time_running) {
    return pair;
  }
  auto cyc = fields.cyc;
  if (fields.cap_user_time_short) {
    cyc = fields.time_cycles + ((cyc - fields.time_cycles) & fields.time_mask);
  }
  const auto shift = static_cast<std::uint64_t>(fields.time_shift);
  // A shift of the type width or more is not a field the kernel writes,
  // and shifting by the type width is undefined, so the pair stands as
  // the page published it.
  constexpr auto kTimeShiftWidth = 64U;
  if (shift >= kTimeShiftWidth) {
    return pair;
  }
  const auto quot = cyc >> shift;
  const auto rem = cyc & ((1ULL << shift) - 1);
  const auto mult = static_cast<std::uint64_t>(fields.time_mult);
  const auto delta =
      fields.time_offset + (quot * mult) + ((rem * mult) >> shift);
  pair.enabled = fields.time_enabled + delta;
  // The running pair moves only where the page names an index, because a
  // page with no index has no running time of its own.
  if (fields.index != 0) {
    pair.running = fields.time_running + delta;
  }
  pair.applied = true;
  return pair;
}

// Opens the context for one event and maps the page the kernel returns.
// The event is bound to `where` exactly as a group member is, so a plan
// pinned to a cpu counts that cpu and a thread-bound plan counts the
// calling thread (FR-024, FR-031). `refusal`, when given, receives the
// reason the kernel gave for refusing, which is the sentence the catalog
// discloses (FR-023).
[[nodiscard]] auto fastContextOpen(int type,
                                   std::uint64_t config,
                                   const Target& where,
                                   std::string* refusal = nullptr)
    -> std::unique_ptr<FastContext>;

// The context's page, read under the mapped-page protocol and returning
// the verdict the gates above produce. The calling thread is the thread
// that opened `context`; any other thread is a contract violation
// (FR-031, FR-040).
[[nodiscard]] auto fastContextRead(const FastContext& context,
                                   std::uint64_t& value) -> FastReadVerdict;

// The context's event page enabled/running pair in nanoseconds, read
// inside the page's own sequence lock. The pair gives a mapped-page
// window its multiplex ratio in every read mode, like the leader's read
// does for a group window (FR-041, FR-040). False when the page carries
// no pair or the sequence moved under the read, and the caller then
// reports a zero pair. The calling thread is the thread that opened
// `context`, on the same contract as `fastContextRead` (FR-031,
// FR-040).
[[nodiscard]] auto fastContextTimePair(const FastContext& context,
                                       std::uint64_t& enabled,
                                       std::uint64_t& running) -> bool;

void fastContextClose(FastContext& context);

// The destructor delegates to the release the library already had, so no
// second release path exists and every owner reaches the same one
// (FR-013). It is defined beside that release, where the platform branch
// has already chosen its body.
inline FastContext::~FastContext()
{
  fastContextClose(*this);
}

// Whether a cpu-target context may be read from the processor the calling
// thread runs on. A thread-bound plan pins nothing and answers true; a
// cpu-pinned context answers true only on the processor it was opened on,
// which is the pinning precondition the fast read carries (FR-045).
[[nodiscard]] auto fastPinningOk(int pinnedCpu,
                                 int currentCpu) noexcept -> bool;

// The corrected decisions that sit behind a syscall only a granted
// `perf_event_open` can reach. Each is a small pure function over values
// the caller already holds, so a registered test drives both arms of each
// on any host, and the coverage gates measure the decision itself. The
// marker over the kernel-facing wrapper stays (FR-046).

/// @brief Whether a group read returned fewer bytes than the group header
/// names (FR-006).
///
/// A leader answering short, or a group read the syscall refuses
/// outright, publishes no count the read never produced, so the action
/// is marked instead. Pure over the two byte counts.
///
/// \pre none
/// \post The verdict is true exactly when the returned count falls below
///       the header size. A negative returned count, which is a read the
///       syscall refused and which produced no count, is reported short.
[[nodiscard]] auto groupReadShort(long returned,
                                  std::size_t headerBytes) noexcept -> bool;

/// @brief The step that turns a pair read's verdict into the values a
/// fold may read (FR-005).
///
/// An unstable pair discloses nothing, so it leaves a zero pair and
/// marks the action; a stable pair publishes the page's own two values.
/// Pure over the verdict and the two current values.
///
/// \pre none
/// \post A stable verdict publishes the page's own two values; an
///       unstable verdict discloses nothing, leaves a zero pair, and
///       reports the refusal.
[[nodiscard]] auto fastPairDisclosed(bool stable,
                                     std::uint64_t enabled,
                                     std::uint64_t running,
                                     std::uint64_t& outEnabled,
                                     std::uint64_t& outRunning) noexcept
    -> bool;

/// @brief Whether a device is the one a vendored row's table scope
/// reaches (FR-019).
///
/// The empty scope and the `core` scope name the core PMU, which a
/// hybrid host publishes as `cpu` plus one device per core class. Any
/// other scope names one device class, spelled either bare or under the
/// kernel's `uncore_` prefix. A row scoped to a class the host publishes
/// no device for reaches no device, so it stays out of the catalog and
/// runs no probe. Pure over the two strings, so a registered test drives
/// every arm without a host that has to grant an event.
///
/// \pre none
/// \post The empty scope and the `core` scope answer true for a core
///       device path alone. Any other scope answers true only for the
///       device of its own class, spelled bare or under the `uncore_`
///       prefix. A scope naming no published device class answers false.
[[nodiscard]] auto scopeReaches(const std::string& devicePath,
                                const std::string& scope) noexcept -> bool;

// What one catalog entry publishes: the read mechanism a plan reads for
// it, and whether the enabled/running pair rides along. The catalog sets
// the fast mode on an entry only where the fast read can succeed for that
// entry, so the host-wide capability verdict never sets the mode on an
// entry whose event the fast instruction cannot read (FR-001).
struct EntryReadSelection
{
  ReadMode mode = ReadMode::SYSCALL;
  bool publishPair = false;
};

/// @brief The selection over the availability probe's verdict and the
/// host's fast-read capability (FR-001, FR-022).
///
/// The catalog sets the fast mode on an entry only where the fast read
/// can succeed for that entry, so the host-wide capability verdict never
/// sets the mode on an entry whose event the fast instruction cannot
/// read. Pure over the two values.
///
/// \pre none
/// \post A countable entry publishes the fast read mode only where the
///       host grants it. Every other state publishes the syscall mode.
///       Only a countable entry publishes the enabled/running pair.
[[nodiscard]] auto entryReadSelectionFor(
    Availability probed, bool fastCapable) noexcept -> EntryReadSelection;

/// @brief The target kinds the two probes settled one entry on (FR-021).
///
/// The availability probe runs once per target kind, and a kind settles
/// the entry exactly where the kernel counted that kind's event. A device
/// that binds one processor for every task runs no per-task probe, so its
/// `SCOPE_REFUSED` verdict settles no kind and the mask names the cpu kind
/// alone. Pure over the two verdicts, so a registered test drives every arm
/// on a host whose cpu-targeted probe is refused, which is what the
/// reference host is (FR-021, FR-022, FR-046).
///
/// \pre none
/// \post A verdict of `countable` names that kind's bit, and every other
///       verdict names no bit. The answer names the cpu bit for a
///       `countable` cpu verdict whatever the per-task verdict is (FR-021).
[[nodiscard]] auto probedKindMask(Availability perTask,
                                  Availability onCpu) noexcept -> TargetMask;

/// @brief The target kinds one catalog entry can be counted on, read from
/// the probe's per-kind verdicts (FR-021).
///
/// The mask names the kinds the probe settled and no others, so a countable
/// entry the cpu-targeted probe refused names no cpu bit, whatever its
/// object is scoped for. `probed_kinds` is the probe's own answer over both
/// kinds, and zero is the answer for a leaf the probe settled no kind on:
/// the enabled and running leaves, which no probe runs for, and every leaf
/// no event provider probed. Those leaves take the object's own scope,
/// which is what the core event source and its hybrid per-core instances
/// decide by admitting both kinds, while a device that binds one processor
/// for every task admits a cpu-target plan alone, and that is what keeps its
/// entries' per-task refusal a scope refusal, apart from an encoding one
/// (FR-021, FR-022). A state no probe settled names no kind, so a refused
/// entry names none however its object is scoped.
///
/// The device-scope rule belongs to the event provider and is read here
/// from the object kind and the canonical path the provider seeded, because
/// FR-021 states the seeding surface gains no target-mask field and no
/// record downstream of it carries one either; an object no event provider
/// seeded publishes clocks and counters, and those count on either kind.
/// The probe's own verdicts reach this function through the table
/// `note_probed_kinds` fills, for the same reason. Pure over the four
/// values, so a registered test reaches every arm without a host that
/// grants an event.
///
/// \pre none
/// \post A state other than `countable` names no target kind. A countable
///       entry the probe settled on one or more kinds names exactly those.
///       A countable entry the probe settled on no kind names the cpu kind
///       alone where an event provider seeded its object and the canonical
///       path is none of the core device paths, and names both kinds
///       everywhere else, an object no event provider seeded among them
///       (FR-021, FR-022).
[[nodiscard]] auto settledTargets(Availability probed,
                                  TargetMask probedKinds,
                                  std::string_view kind,
                                  const std::string& path) noexcept
    -> TargetMask;

/// @brief The state a catalog entry settles on when no probe granted it
/// (FR-021).
///
/// A device that binds one processor for every task refuses the per-task
/// kind by its own scope, so the catalog publishes that refusal for the
/// kind. The cpu probe's verdict belongs to the cpu kind. A permission
/// refusal names the scope, and every other verdict names its own cause,
/// which is what keeps an encoding refusal readable apart from a scope one
/// (FR-021, FR-022, US4/AC8). An object no device scope owns has no such
/// refusal to publish and settles on the verdict itself. Pure over the two
/// values, so a registered test drives both arms on any host.
///
/// \pre none
/// \post A permission verdict on a device-scoped device settles on
///       `SCOPE_REFUSED`. Every other verdict, on a device-scoped device or
///       not, settles on itself.
[[nodiscard]] auto scopeSettledState(
    Availability onCpu, bool deviceScoped) noexcept -> Availability;

/// @brief Whether a catalog entry's own state lets the request open a
/// provider window (FR-021, FR-024).
///
/// A leaf the catalog reports as not countable now is a construction
/// error, refused before any provider window opens and before any
/// hardware read. One state is not the caller's to clear: a device that
/// binds one processor for every task refuses the per-task kind by its
/// own scope and publishes `SCOPE_REFUSED` for it, so a request naming
/// the cpu kind does not ask for the kind the scope refused. Such a
/// request proceeds to the provider window, where the kernel's own
/// verdict for the cpu-targeted event belongs; a refusal there names the
/// window, and the catalog state stays the one it settled on (FR-022,
/// FR-024, SC-007).
/// Pure over the two values, so a registered test drives every arm on a
/// host whose catalog publishes no scope-refused entry, which is what
/// the reference host does: a cpu-targeted probe there answers `EINVAL`,
/// and the probe settles that answer as `not_encodable`.
///
/// \pre none
/// \post A countable entry answers true under either kind. A
///       `SCOPE_REFUSED` entry answers true for the cpu kind alone, and
///       answers false for the per-task kind. Every other state answers
///       false under either kind.
[[nodiscard]] auto availabilityGatePasses(
    Availability probed, TargetKind requested) noexcept -> bool;

// The fast-mode window (group_io.cpp, FR-040): one context per member
// leaf, the enabled/running pair taken from the leader's page. Null
// when a leaf names no device, no countable entry, or a context the
// kernel refuses.
[[nodiscard]] auto pmuOpenFastWindow(const PmuState& state,
                                     const LeafSet& leaves,
                                     const Target& where)
    -> std::unique_ptr<WindowReader>;

// The decisions the seam reaches so a fixture can drive them with a
// device the reference host does not publish. `probeDevice` takes a
// device and settles each entry's countability per target kind,
// `loadDevice` takes a device directory and reads one, and
// `mergeVendored` places the vendored rows. All three sit in this
// namespace so a registered test can supply a device the running kernel
// never lists, which is what reaches a hybrid per-core scope.
void probeDevice(PmuDevice& device, bool fastCapable);

// The fast verdict over one event page. The header publishes the
// capability as the `cap_user_rdpmc` bit, and that bit is the verdict
// (FR-017).
[[nodiscard]] auto pageGrantsUserRdpmc(std::uint64_t cap_user_rdpmc) noexcept
    -> bool;

// The fast verdict of one device, read from an event page that device
// opened. A device whose open the kernel refuses publishes no fast
// verdict (FR-017).
[[nodiscard]] auto devicePageFastVerdict(const PmuDevice& device) -> bool;

[[nodiscard]] auto loadDevice(const std::filesystem::path& dir)
    -> std::optional<PmuDevice>;

/// @brief Places the vendored rows whose scope reaches `device` (FR-019).
///
/// The placement step, which adds every row of the selected architecture
/// table whose Unit scope names `device`'s own class: a core-scoped row
/// lands on each core device a hybrid host publishes, an uncore-scoped
/// row on the device of its class, and a row scoped to a class this host
/// publishes no device for reaches none. A kernel alias of the same name
/// keeps the kernel's own entry. It is exposed here so a registered test
/// can drive placement over a device the running kernel never publishes,
/// which is the only way a placement arm runs off a host that owns the
/// hardware.
void mergeVendored(PmuDevice& device);

}  // namespace sg::counters::detail
