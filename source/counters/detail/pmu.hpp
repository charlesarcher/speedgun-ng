#pragma once

// Private seam of the Linux PMU provider (specs/007-counters-and-timers,
// US6, T045..T048). Never installed. The seam separates table parsing
// (simdjson-private TU: table_parse.cpp), encoding (encode.cpp), and
// group I/O (group_io.cpp) from the provider assembly
// (provider.cpp). FR-013/004 privacy: the sole <simdjson.h> include of
// source/counters/ lives in table_parse.cpp; nothing here leaks JSON
// types.

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"

namespace sg::counters::detail
{

// One architecture event-table entry, semantic fields only: the
// encoder composes them with the running kernel's bit layouts
// (FR-037). Metric-definition rows are not entries: they ship as
// catalog data for a future release; they are no countables (US6).
struct pmu_table_entry
{
  std::string name;
  std::string description;
  std::string unit = "none";
  // Semantic config fields: format-field name to value. The
  // convention-mapped names: "event" (EventCode), "umask" (UMask),
  // plus any other JSON key that names a format field.
  std::vector<std::pair<std::string, std::uint64_t>> fields;
};

// CPU identification read once at open (FR-038): vendor string, family
// decimal, model hex, matching the upstream mapping file's spelling.
struct pmu_ident
{
  std::string vendor;
  int family = -1;
  int model = -1;
};

// The host's CPU identification (table_parse.cpp; CPUID-based, cached).
[[nodiscard]] auto pmu_ident_current() -> pmu_ident;

// The architecture directory the mapping file selects for `id`: the
// first row whose regex matches, parsed once lazily (FR-038). Empty
// when no row matches; parse failures surface as an empty table.
[[nodiscard]] auto pmu_select_directory(const pmu_ident& id) -> std::string;

// The parsed event table for `directory`, lazily and once per
// directory (FR-038). Entries carry semantic fields only.
[[nodiscard]] auto pmu_load_table(const std::string& directory)
    -> const std::vector<pmu_table_entry>&;

// One bit range of a sysfs format field: which config word, inclusive
// low and high bit.
struct format_range
{
  int config_word = 0;
  int low = 0;
  int high = 0;
};

// The bit layout of one named format field (encode.cpp): parses
// "config:0-7,32-35" and friends. False when the file does not
// describe a config-bit field.
[[nodiscard]] auto parse_format_field(std::string_view spec,
                                      std::vector<format_range>& out) -> bool;

// Compose one entry's config words from semantic fields and the
// kernel's format layouts: every referenced field must exist, else
// nothing is half-composed and the entry is `not_encodable`
// (FR-037). On success `words` holds the per-config-word values.
[[nodiscard]] auto pmu_compose_config(
    const std::vector<std::pair<std::string, std::uint64_t>>& fields,
    const std::vector<std::pair<std::string, std::vector<format_range>>>&
        formats,
    std::vector<std::pair<int, std::uint64_t>>& words) -> bool;

// Probe one config through a test-open against the PMU `type`
// (provider.cpp, FR-039): the availability the kernel grants this
// caller for this event. The probe answers `countable`,
// `not_encodable`, or `permission_blocked`, which is the closed set of
// states a pmu catalog entry carries. `availability::absent` belongs to
// a provider that declares a leaf absent from the object it seeds, and
// a device the kernel does not publish is dropped at discovery, so no
// entry carries it.
[[nodiscard]] auto pmu_probe(
    int type, const std::vector<std::pair<int, std::uint64_t>>& words)
    -> availability;

// One catalog entry the provider built: the composed config words, the
// description, and the probed availability (FR-037, FR-039).
struct pmu_entry
{
  std::string name;
  std::string description;
  // Composed per-config-word values. Empty for the enabled and running
  // leaves and for an entry the encoder refused, so `is_time_pair`
  // tells the two apart.
  std::vector<std::pair<int, std::uint64_t>> words;
  bool is_time_pair = false;
  availability avail = availability::not_encodable;
  read_mode mode = read_mode::syscall;
};

// One event-source device: its canonical path, its PMU type, the bit
// layouts it publishes, and the merged catalog in seed order (kernel
// aliases first, then the vendored table; a kernel alias wins a name
// conflict, FR-037).
struct pmu_device
{
  std::string path;  // canonical spelling, the sysfs device name
  std::string description;
  int type = -1;
  std::vector<std::pair<std::string, std::vector<format_range>>> formats;
  std::vector<pmu_entry> entries;
  // True when the device discloses the enabled/running time pair as
  // ordinary leaves, which the fold layer reads for the multiplex
  // ratio (FR-019, FR-041, C-PRO-6).
  bool has_time_pair = false;
};

// The released provider state (T047). Off Linux every device list is
// empty and the catalog is the whole reduced difference (FR-042).
struct pmu_state
{
  std::vector<pmu_device> devices;
  // Fast-read probe verdict (FR-023, R-011): the mapped user-access
  // page is readable and publishes a usable counter index, or the
  // refusal is named for the skip report.
  bool fast_available = false;
  std::string fast_refusal;
};

// Opens the syscall-mode window for `leaves` (group_io.cpp, FR-041):
// one group per device, one read per group leader per sampling action.
// Null when a leaf names no device or no countable entry.
[[nodiscard]] auto pmu_open_window(const pmu_state& state,
                                   const leaf_set& leaves,
                                   const target& where)
    -> std::unique_ptr<window_reader>;

// The fast-read probe verdict (fast_read.cpp, FR-023, R-011). Written
// into `state`; the catalog discloses the achieved mode.
void pmu_probe_fast(pmu_state& state);

// The outcome of one mapped-page read attempt (FR-040).
enum class fast_read_verdict : std::uint8_t
{
  ok,
  not_allowed,  // no capability, no valid index, or a foreign thread
  unstable  // the page sequence moved; the caller retries
};

// The width the kernel publishes counters at for user counter reads.
constexpr std::uint32_t kRnpmcCounterWidth = 48;

// The decode half of the protocol, over values the caller already
// sampled: the page sequence before and after, the capability word, the
// raw instruction result, the kernel offset, and the counter width. The
// caller gates the capability ahead of the instruction these values come
// from, so a caller with no capability never pays for the read
// (FR-040, R-011).
[[nodiscard]] auto fast_decode(std::uint32_t sequence_before,
                               std::uint32_t sequence_after,
                               std::uint64_t capability,
                               std::uint64_t raw,
                               std::int64_t offset,
                               std::uint32_t width,
                               std::uint64_t& value) -> fast_read_verdict;

// One per-thread fast-read context: the event file descriptor, its
// mapped page, the shared user counter page, and the owning thread. A
// context belongs to the thread that opened it (FR-031, FR-040).
struct fast_context
{
  int fd = -1;
  void* map = nullptr;
  void* user = nullptr;
  std::size_t user_length = 0;
  std::thread::id owner {};
};

[[nodiscard]] auto fast_context_open(int type, std::uint64_t config)
    -> std::unique_ptr<fast_context>;

[[nodiscard]] auto fast_context_read(const fast_context& context,
                                     std::uint64_t& value) -> fast_read_verdict;

// The context's event page enabled/running pair in nanoseconds, read
// inside the page's own sequence lock. The pair gives a mapped-page
// window its multiplex ratio in every read mode, like the leader's read
// does for a group window (FR-041, FR-040). False when the page carries
// no pair or the sequence moved under the read, and the caller then
// reports a zero pair.
[[nodiscard]] auto fast_context_time_pair(const fast_context& context,
                                          std::uint64_t& enabled,
                                          std::uint64_t& running) -> bool;

void fast_context_close(fast_context& context);

// The fast-mode window (group_io.cpp, FR-040): one context per member
// leaf, the enabled/running pair taken from the leader's page. Null
// when a leaf names no device, no countable entry, or a context the
// kernel refuses.
[[nodiscard]] auto pmu_open_fast_window(const pmu_state& state,
                                        const leaf_set& leaves,
                                        const target& where)
    -> std::unique_ptr<window_reader>;

}  // namespace sg::counters::detail
